#pragma once

#include <memory>
#include <unordered_map>


#include "../Alerts/AlertTags.h" 

using std::string;

namespace OptionDB {

	namespace TagDBInterface {
		struct PairHash {
			template <class T1, class T2>
			std::size_t operator() (const std::pair<T1, T2>& p) const {
				auto h1 = std::hash<T1>{}(p.first);
				auto h2 = std::hash<T2>{}(p.second);
				return h1 ^ h2;
			}
		};


		class AlertTagDBInterface {
		public:
			AlertTagDBInterface() { initialize(); }

			std::unordered_map<std::pair<string, string>, int, PairHash> tagInterface() { return tagToInt; }
			static std::unordered_map<std::pair<string, string>, int, PairHash> tagToInt;
			// Reverse of map to use table keys as map keys
			static std::unordered_map<int, std::pair<string, string>> intToTag;

			static void initialize() {
				// Fill the intToTagMap
				for (const auto& key : tagToInt) {
					intToTag[key.second] = key.first;
				}
			}

		};

		std::unordered_map<std::pair<string, string>, int, PairHash> AlertTagDBInterface::tagToInt = {
			{ {Alerts::EnumString::option_type(Alerts::OptionType::Call), "OptionType"}, 1 },
				{ {Alerts::EnumString::option_type(Alerts::OptionType::Put), "OptionType"}, 2 },

				{ {time_frame(TimeFrame::FiveSecs), "TimeFrame"}, 3 },
				{ {time_frame(TimeFrame::ThirtySecs), "TimeFrame"}, 4 },
				{ {time_frame(TimeFrame::OneMin), "TimeFrame"}, 5 },
				{ {time_frame(TimeFrame::FiveMin), "TimeFrame"}, 6 },

				{ {Alerts::EnumString::relative_to_money(Alerts::RelativeToMoney::ATM), "RelativeToMoney"}, 7 },
				{ {Alerts::EnumString::relative_to_money(Alerts::RelativeToMoney::ITM1), "RelativeToMoney"}, 8 },
				{ {Alerts::EnumString::relative_to_money(Alerts::RelativeToMoney::ITM2), "RelativeToMoney"}, 9 },
				{ {Alerts::EnumString::relative_to_money(Alerts::RelativeToMoney::ITM3), "RelativeToMoney"}, 10 },
				{ {Alerts::EnumString::relative_to_money(Alerts::RelativeToMoney::ITM4), "RelativeToMoney"}, 11 },
				{ {Alerts::EnumString::relative_to_money(Alerts::RelativeToMoney::DeepITM), "RelativeToMoney"}, 12 },
				{ {Alerts::EnumString::relative_to_money(Alerts::RelativeToMoney::OTM1), "RelativeToMoney"}, 13 },
				{ {Alerts::EnumString::relative_to_money(Alerts::RelativeToMoney::OTM2), "RelativeToMoney"}, 14 },
				{ {Alerts::EnumString::relative_to_money(Alerts::RelativeToMoney::OTM3), "RelativeToMoney"}, 15 },
				{ {Alerts::EnumString::relative_to_money(Alerts::RelativeToMoney::OTM4), "RelativeToMoney"}, 16 },
				{ {Alerts::EnumString::relative_to_money(Alerts::RelativeToMoney::DeepOTM), "RelativeToMoney"}, 17 },

				{ {Alerts::EnumString::time_of_day(Alerts::TimeOfDay::Hour1), "TimeOfDay"}, 18 },
				{ {Alerts::EnumString::time_of_day(Alerts::TimeOfDay::Hour2), "TimeOfDay"}, 19 },
				{ {Alerts::EnumString::time_of_day(Alerts::TimeOfDay::Hour3), "TimeOfDay"}, 20 },
				{ {Alerts::EnumString::time_of_day(Alerts::TimeOfDay::Hour4), "TimeOfDay"}, 21 },
				{ {Alerts::EnumString::time_of_day(Alerts::TimeOfDay::Hour5), "TimeOfDay"}, 22 },
				{ {Alerts::EnumString::time_of_day(Alerts::TimeOfDay::Hour6), "TimeOfDay"}, 23 },
				{ {Alerts::EnumString::time_of_day(Alerts::TimeOfDay::Hour7), "TimeOfDay"}, 24 },

				{ {Alerts::EnumString::vol_st_dev(Alerts::VolumeStDev::Over1), "VolumeStDev"}, 25 },
				{ {Alerts::EnumString::vol_st_dev(Alerts::VolumeStDev::Over2), "VolumeStDev"}, 26 },
				{ {Alerts::EnumString::vol_st_dev(Alerts::VolumeStDev::Over3), "VolumeStDev"}, 27 },
				{ {Alerts::EnumString::vol_st_dev(Alerts::VolumeStDev::Over4), "VolumeStDev"}, 28 },
				{ {Alerts::EnumString::vol_st_dev(Alerts::VolumeStDev::LowVol), "VolumeStDev"}, 29 },

				{ {Alerts::EnumString::vol_threshold(Alerts::VolumeThreshold::Vol100), "VolumeThreshold"}, 30 },
				{ {Alerts::EnumString::vol_threshold(Alerts::VolumeThreshold::Vol250), "VolumeThreshold"}, 31 },
				{ {Alerts::EnumString::vol_threshold(Alerts::VolumeThreshold::Vol500), "VolumeThreshold"}, 32 },
				{ {Alerts::EnumString::vol_threshold(Alerts::VolumeThreshold::Vol1000), "VolumeThreshold"}, 33 },
				{ {Alerts::EnumString::vol_threshold(Alerts::VolumeThreshold::LowVol), "VolumeThreshold"}, 34 },

				{ {Alerts::EnumString::price_delta(Alerts::PriceDelta::Under1), "UnderlyingPriceDelta"}, 35 },
				{ {Alerts::EnumString::price_delta(Alerts::PriceDelta::Under2), "UnderlyingPriceDelta"}, 36 },
				{ {Alerts::EnumString::price_delta(Alerts::PriceDelta::Over2), "UnderlyingPriceDelta"}, 37 },

				{ {Alerts::EnumString::price_delta(Alerts::PriceDelta::Under1), "OptionPriceDelta"}, 38 },
				{ {Alerts::EnumString::price_delta(Alerts::PriceDelta::Under2), "OptionPriceDelta"}, 39 },
				{ {Alerts::EnumString::price_delta(Alerts::PriceDelta::Over2), "OptionPriceDelta"}, 40 },

				{ {Alerts::EnumString::daily_highs_and_lows(Alerts::DailyHighsAndLows::NDL), "UnderlyingDailyHighsAndLows"}, 41 },
				{ {Alerts::EnumString::daily_highs_and_lows(Alerts::DailyHighsAndLows::NDH), "UnderlyingDailyHighsAndLows"}, 42 },
				{ {Alerts::EnumString::daily_highs_and_lows(Alerts::DailyHighsAndLows::Inside), "UnderlyingDailyHighsAndLows"}, 43 },

				{ {Alerts::EnumString::daily_highs_and_lows(Alerts::DailyHighsAndLows::NDL), "OptionDailyHighsAndLows"}, 44 },
				{ {Alerts::EnumString::daily_highs_and_lows(Alerts::DailyHighsAndLows::NDH), "OptionDailyHighsAndLows"}, 45 },
				{ {Alerts::EnumString::daily_highs_and_lows(Alerts::DailyHighsAndLows::Inside), "OptionDailyHighsAndLows"}, 46 },

				{ {Alerts::EnumString::local_highs_and_lows(Alerts::LocalHighsAndLows::NLL), "UnderlyingLocalHighsAndLows"}, 47 },
				{ {Alerts::EnumString::local_highs_and_lows(Alerts::LocalHighsAndLows::NLH), "UnderlyingLocalHighsAndLows"}, 48 },
				{ {Alerts::EnumString::local_highs_and_lows(Alerts::LocalHighsAndLows::Inside), "UnderlyingLocalHighsAndLows"}, 49 },

				{ {Alerts::EnumString::local_highs_and_lows(Alerts::LocalHighsAndLows::NLL), "OptionLocalHighsAndLows"}, 50 },
				{ {Alerts::EnumString::local_highs_and_lows(Alerts::LocalHighsAndLows::NLH), "OptionLocalHighsAndLows"}, 51 },
				{ {Alerts::EnumString::local_highs_and_lows(Alerts::LocalHighsAndLows::Inside), "OptionLocalHighsAndLows"}, 52 },
		}


	}
}