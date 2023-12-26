#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include <iostream>
#include <unordered_set>
#include <queue>
#include <vector>
#include <memory>
#include <chrono>

//#include "tWrapper.h"
#include "MockWrapper.h"
#include "MockClient.h"
#include "ContractData.h"

namespace Securities {

	struct Security {
		Security(IBString ticker, int multiple, std::string secType);

		IBString ticker;
		Contract con;
		Contract callContract;
		Contract putContract;
		int multiple;
		std::string secType;
	};

	class SecurityRequestHandler {
	public:
		SecurityRequestHandler(Security sec, int req, int numStrikes, IBString todayDate);

		// Mutators
		int numReqs() const;
		std::vector<int> currentActiveReqs() const;
		bool checkCurrentScope(const int req);

		void changeNumStrikes(const int strikes);
		std::vector<int> getStrikes(const double price);

		void initializeOptionRequests(MockClient& EC, const int mktReq);
		void updateOptionRequests(MockClient& EC, const double curPrice, IBString todayDate,
			std::shared_ptr<std::unordered_map<int, std::shared_ptr<ContractData>>> chainData);

		bool strikesUpdated{ false };


	private:
		Security sec_;
		int req_;
		int numStrikes_;
		IBString todayDate_;

		std::vector<int> currentReqs_;
		std::unordered_set<int> consInScope_;

		std::mutex secMtx;
	};
}