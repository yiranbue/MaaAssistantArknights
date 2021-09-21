﻿#include "InfrastPowerTask.h"

#include "WinMacro.h"
#include "Identify.h"
#include "Configer.h"

bool asst::InfrastPowerTask::run()
{
	if (m_controller_ptr == nullptr
		|| m_identify_ptr == nullptr)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	json::value task_start_json = json::object{
		{ "task_type",  "InfrastPowerTask" },
		{ "task_chain", m_task_chain}
	};
	m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

	bool is_the_left = false;
	for (int i = 0; i != PowerNum; ++i) {
		if (need_exit()) {
			return false;
		}
		swipe_left();
		if (!enter_station({ "Power", "PowerMini" }, i)) {
			return false;
		}
		json::value enter_json;
		enter_json["station"] = "Power";
		enter_json["index"] = i;
		m_callback(AsstMsg::EnterStation, enter_json, m_callback_arg);

		if (enter_operator_selection()) {
			m_callback(AsstMsg::ReadyToShift, enter_json, m_callback_arg);
			select_operators(!is_the_left);
			if (!is_the_left) {
				is_the_left = true;
			}
			m_callback(AsstMsg::ShiftCompleted, enter_json, m_callback_arg);
		}
		else {
			m_callback(AsstMsg::NoNeedToShift, enter_json, m_callback_arg);
		}
		if (!sleep(1000)) {
			return false;
		}
		click_return_button();
		if (!sleep(1000)) {
			return false;
		}
	}
	m_callback(AsstMsg::TaskCompleted, task_start_json, m_callback_arg);
	return true;
}

bool asst::InfrastPowerTask::enter_operator_selection()
{
	// 有这些文字之一就说明“进驻信息”这个按钮已经点开了
	static constexpr std::array<std::string_view, 3> info_opened_flags = {
		"当前房间入住信息","进驻人数","清空"
	};

	std::vector<TextArea> ocr_result = ocr_detect();

	bool is_info_opened =
		std::find_first_of(
			ocr_result.cbegin(), ocr_result.cend(),
			info_opened_flags.cbegin(), info_opened_flags.cend(),
			[](const TextArea& lhs, const std::string_view& rhs)
			-> bool { return lhs.text == rhs; })
		!= ocr_result.cend();

	static constexpr std::string_view station_info = "进驻信息";
	// 如果“进驻信息”窗口没点开，那就点开
	if (!is_info_opened) {
		auto station_info_iter = std::find_if(ocr_result.cbegin(), ocr_result.cend(),
			[](const TextArea& textarea) -> bool {
				return textarea.text == station_info;
			});
		m_controller_ptr->click(station_info_iter->rect);
		sleep(1000);
	}

	cv::Mat image = m_controller_ptr->get_image();
	auto goin_result = m_identify_ptr->find_image(image, "GoIn");
	// 如果找到了GoIn（“进驻”按钮），进点进去，准备选择干员
	if (goin_result.score > Configer::TemplThresholdDefault) {
		m_controller_ptr->click(goin_result.rect);
		return sleep(1000);
	}
	else {
		return false;	// 否则说明这个发电站是有人在的，不用换班
	}
}

int asst::InfrastPowerTask::select_operators(bool need_to_the_left)
{
	bool ret = false;
	if (need_to_the_left) {
		ret = swipe_to_the_left();
	}
	// 发电站干员不用做识别，直接选择第一个即可
	ret &= click_first_operator();
	ret &= click_confirm_button();
	if (!ret) {
		return -1;
	}

	// 发电站固定只选择一个干员，return 1
	return 1;
}