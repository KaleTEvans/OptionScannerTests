#include "MockSecurityReqHandler.h"

namespace Securities {

	Security::Security(IBString ticker, int multiple, std::string secType) :
		ticker(ticker), multiple(multiple), secType(secType) 
	{
		// Create primary contract for underlying
		con.symbol = ticker;
		con.secType = secType;
		con.currency = "USD";
		con.primaryExchange = "CBOE";

		// Create option contracts
		callContract.symbol = ticker;
		callContract.secType = "OPT";
		callContract.currency = "USD";
		callContract.exchange = "CBOE";
		callContract.right = "CALL";

		putContract.symbol = ticker;
		putContract.secType = "OPT";
		putContract.currency = "USD";
		putContract.exchange = "CBOE";
		putContract.right = "PUT";
	}

	SecurityRequestHandler::SecurityRequestHandler(Security sec, int req, int numStrikes, IBString todayDate) :
		sec_(sec), req_(req), numStrikes_(numStrikes), todayDate_(todayDate) {}

	int SecurityRequestHandler::numReqs() const { return currentReqs_.size(); }
	std::vector<int> SecurityRequestHandler::currentActiveReqs() const { return currentReqs_; }
	bool SecurityRequestHandler::checkCurrentScope(const int req) { return consInScope_.find(req) != consInScope_.end(); }

	void SecurityRequestHandler::changeNumStrikes(const int strikes) { numStrikes_ = strikes; }

	void SecurityRequestHandler::initializeOptionRequests(MockClient& EC, const int mktReq) {
		// Send an RTB request for underlying index
		EC.reqRealTimeBars
		(req_
			, sec_.con
			, 5
			, ""
			, UseRTH::OnlyRegularTradingData
		);

		// Now add the exchange and send a mkt data request
		//con_.exchange = *Exchange::IB_SMART;
		//EC.reqMktData(mktReq, con_, "", true);
	}

	void SecurityRequestHandler::updateOptionRequests(MockClient& EC, const double curPrice, IBString todayDate,
		std::shared_ptr<std::unordered_map<int, std::shared_ptr<ContractData>>> chainData) {

		std::vector<int> strikes = getStrikes(curPrice);

		// Request queue gets filled by new strikes
		std::queue<Contract> reqQueue;

		// Clear contractsInScope set each time strikes are updated to ensure newly populated strikes always are in scope
		consInScope_.clear();

		for (auto i : strikes) {
			// If the contracts map doesn't already contain the strike, then a new one has come into scope
			if (chainData->find(i) == chainData->end()) {
				// Create new contracts if not in map and add to queue for requests
				//Contract con;
				//con.symbol = ticker_;
				//con.secType = "OPT";
				//con.currency = "USD";
				//con.exchange = "SMART";
				//con.primaryExchange = "CBOE";
				//con.lastTradeDateOrContractMonth = todayDate;
				//con.strike = i;
				//con.right = "CALL";

				//// Insert into the request queue
				//reqQueue.push(con);

				//// Now change contract to put and insert
				//con.right = "PUT";
				//reqQueue.push(con);

				Contract callCon = sec_.callContract;
				Contract putCon = sec_.putContract;
				callCon.strike = i;
				putCon.strike = i;
				reqQueue.push(callCon);
				reqQueue.push(putCon);
			}

			// Update the contracts being tracked within the scope
			consInScope_.insert(i);
			consInScope_.insert(i + 1);
		}

		// if (!reqQueue.empty()) OPTIONSCANNER_INFO("{} new contracts added to request queue", reqQueue.size());

		// Now empty the queue and create the requests
		while (!reqQueue.empty()) {
			Contract con = reqQueue.front();

			int req = 0; // Ends in 0 or 5 if call, 1 or 6 for puts
			if (con.right == "CALL") req = static_cast<int>(con.strike);
			else if (con.right == "PUT") req = static_cast<int>(con.strike + 1);

			// Now create the request
			EC.reqRealTimeBars
			(req
				, con
				, 5
				, ""
				, UseRTH::OnlyRegularTradingData
			);

			// Update request tracker
			currentReqs_.push_back(req);

			reqQueue.pop();
		}

		// OPTIONSCANNER_DEBUG("New requests sent to queue, total option active requests: {}", currentReqs_.size());
		strikesUpdated = true;
	}

	std::vector<int> SecurityRequestHandler::getStrikes(const double price) {
		std::vector<int> strikes;

		// Round the price down to nearest increment
		int roundedPrice = int(price + (sec_.multiple / 2));
		roundedPrice -= roundedPrice % sec_.multiple;
		int strikePrice = roundedPrice - (sec_.multiple * 5);

		// This will give us 2 times the number of strikes provided
		while (strikePrice <= roundedPrice + (sec_.multiple * numStrikes_)) {
			strikes.push_back(strikePrice);
			strikePrice += sec_.multiple;
		}

		for (auto i : strikes) std::cout << i << " ";
		std::cout << std::endl;

		return strikes;
	}
}