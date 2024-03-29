//#include "pch.h"
#include "MockOptionScanner.h"

MockOptionScanner::MockOptionScanner(int delay, bool useDBM) : delay_(delay), useDBM(useDBM), EC(YW) {

	// Request last quote for SPX upon class initiation to get closest option strikes
	/*SPX.symbol = "SPX";
	SPX.secType = "IND";
	SPX.currency = "USD";*/

	EC.setCandleInterval(delay);

	// Create RTB request for SPX underlying **This will not be accessible until buffer is processed
	/*EC.reqRealTimeBars
	(1234
		, SPX
		, 0
		, ""
		, true
	);*/

	// Add SPX to the added contracts set
	//addedContracts.push_back(1234);

	// Start the dbm
	dbm = std::make_shared<OptionDB::DatabaseManager>(false);
	if (useDBM) dbm->start();

	//SPX_.initializeOptionRequests(EC, 111);

	// Initialzie the contract chain
	contractChain_ = std::make_shared<std::unordered_map<int, std::shared_ptr<ContractData>>>();
}

void MockOptionScanner::startUnderlyingStream() {
	SPX.symbol = "SPX";
	SPX.secType = "IND";
	SPX.currency = "USD";

	EC.reqRealTimeBars
	(1234
		, SPX
		, 0
		, ""
		, true
	);

	// Add SPX to the added contracts set
	addedContracts.push_back(1234);

	underlyingStreamStarted = true;
}

// Test function Accessors
int MockOptionScanner::checkContractMap() { return contractChain_->size(); }
double MockOptionScanner::currentSPX() { return (currentSPX_); }
int MockOptionScanner::diffChainSize() { return (curChainSize_ - prevChainSize_); }
int MockOptionScanner::prevBufferCapacity() { return prevBufferCapacity_; }
std::vector<int> MockOptionScanner::finalContractCt() { return addedContracts; }
std::shared_ptr<std::unordered_map<int, std::shared_ptr<ContractData>>> MockOptionScanner::getContractChain() { return contractChain_; }


//============================================================
// Open Market Data Processing Funtions
//============================================================

void MockOptionScanner::streamOptionData() {

	currentSPX_ = YW.getSPXPrice();
	updateStrikes(currentSPX_);
	//SPX_.updateOptionRequests(EC, currentSPX_, "", contractChain_);

	// Add temporary comparison value for Chain
	prevChainSize_ = 19;


	//==========================================================================
	// This while loop is important, as it will be open the entire day, and 
	// collect all options data and be responsible for sending alerts and all
	// other info
	//==========================================================================

	while (YW.notDone()) {

		// Use the wrapper conditional to check buffer
		std::unique_lock<std::mutex> lock(YW.getWrapperMutex());
		YW.getWrapperConditional().wait(lock, [&] { return YW.checkMockBufferFull(); });

		// Update test variables
		prevBufferCapacity_ = YW.getBufferCapacity();

		for (auto& candle : YW.getProcessedFiveSecCandles()) {

			int req = candle->reqId();

			if (contractChain_->find(req) != contractChain_->end()) {
				contractChain_->at(req)->updateData(std::move(candle));
				
			}
			else {
				std::shared_ptr<ContractData> cd;
				if (req == 1234 && useDBM) {
					cd = std::make_shared<ContractData>(req, dbm);
				}
				else {
					cd = std::make_shared<ContractData>(req);
				}

				registerAlertCallback(cd);
				cd->updateData(std::move(candle));
				contractChain_->insert({ req, cd });
			}
		}

		// Update test variables
		curChainSize_ = contractChain_->size();
		currentSPX_ = contractChain_->at(1234)->currentPrice();
		// std::cout << "Buffer size currently at: " << YW.getBufferCapacity() << std::endl;
		strikesUpdated = true;

		lock.unlock();
		mosCnditional.notify_one();

		updateStrikes(contractChain_->at(1234)->currentPrice());
		//SPX_.updateOptionRequests(EC, contractChain_->at(1234)->currentPrice(), "", contractChain_);
		
	}

	if (!YW.notDone()) {
		std::cout << "Wrapper notified done. Terminating." << std::endl;
		EC.cancelRealTimeBars();

		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (useDBM) dbm->stop();
	}
}

//==============================================================
// Alert Functions
//==============================================================

void MockOptionScanner::registerAlertCallback(std::shared_ptr<ContractData> cd) {
	cd->registerAlert([this, cd](std::shared_ptr<CandleTags> ct) {
		std::lock_guard<std::mutex> lock(optScanMtx);
		//std::cout << "Callback from: " << ct->candle.reqId() << " At " << ct->candle.time() << std::endl;

		// Add underlying specific tags
		try {
			double underlyingPrice = contractChain_->at(1234)->currentPrice();
			Alerts::PriceDelta pd = contractChain_->at(1234)->priceDelta(ct->getTimeFrame());
			Alerts::DailyHighsAndLows dhl = contractChain_->at(1234)->dailyHLComparison();
			Alerts::LocalHighsAndLows lhl = contractChain_->at(1234)->localHLComparison();
			Alerts::RelativeToMoney rtm = distFromPrice(cd->optType(), cd->strikePrice(), underlyingPrice);
			ct->addUnderlyingTags(rtm, pd, dhl, lhl);
		}
		catch (const std::exception& e) {
			std::cout << "Issue with callback: " << e.what() << std::endl;
		}
		// Send to dbm
		//std::cout << "Sending to DBM" << std::endl;
		if (useDBM) dbm->addToInsertionQueue(ct);
		
	});
}

//==============================================================
// Option Chain Update Helper Functions
//==============================================================

void MockOptionScanner::updateStrikes(double price) {

	std::lock_guard<std::mutex> lock(optScanMutex_);

	vector<int> strikes = populateStrikes(price);

	// Clear contractsInScope set each time strikes are updated to ensure newly populated strikes always are in scope
	contractsInScope.clear();

	for (auto i : strikes) {
		// If the contracts map doesn't already contain the strike, then a new one has come into scope
		if (contractChain_->find(i) == contractChain_->end()) {

			// Create new contracts if not in map and add to queue for requests
			Contract con;
			con.symbol = "SPX";
			con.secType = "OPT";
			con.currency = "USD";
			con.strike = i;
			con.right = "CALL";

			// Insert into queue
			contractReqQueue.push(con);

			// Now change to put and insert
			con.right = "PUT";
			contractReqQueue.push(con);
		}

		// Update the contracts in the scope
		contractsInScope.insert(i);
		contractsInScope.insert(i + 1);
	}

	// Empty queue and create the requests
	while (!contractReqQueue.empty()) {
		if (!underlyingStreamStarted) startUnderlyingStream();
		Contract con = contractReqQueue.front();

		int req = 0; // Ends in 0 or 5 if call, 1 or 6 for puts
		if (con.right == "CALL") req = static_cast<int>(con.strike);
		if (con.right == "PUT") req = static_cast<int>(con.strike + 1);

		// std::cout << "Popping from queue Req: " << req << std::endl;

		// Now create the request
		EC.reqRealTimeBars
		(req
			, con
			, 0
			, ""
			, true
		);

		// Update contract request vector
		addedContracts.push_back(req);

		contractReqQueue.pop();
	}

	// If addedContracts vector exceeds current buffer size, update the buffer
	if (static_cast<int>(addedContracts.size()) > YW.getBufferCapacity()) YW.setBufferCapacity(static_cast<int>(addedContracts.size()));
	//OPTIONSCANNER_DEBUG("Buffer capacity updated. Now at {}", YW.candleBuffer.checkBufferCapacity());
	strikesUpdated = true;
}

// Wait on dbm to finish inserting to db
void MockOptionScanner::waitForDBM() {
	std::unique_lock<std::mutex> lock(dbm->getMtx());
	dbm->getCV().wait(lock, [&]() {
		return dbm->processingComplete();
	});
}

int MockOptionScanner::getUnderlyingDBCount() { return dbm->getUnderlyingCount(); }
int MockOptionScanner::getOptionDBCount() { return dbm->getOptionCount(); }

//=====================================================
// Helper Functions
//=====================================================

vector<int> populateStrikes(double price) {
	vector<int> strikes;

	int multiple = 5; // SPX Strikes are in increments of 5
	// Round the price down to nearest increment
	int roundedPrice = int(price + (multiple / 2));
	roundedPrice -= roundedPrice % multiple;
	int strikePrice = roundedPrice - (multiple * 4);

	// This will give us 9 strikes in total
	while (strikePrice <= roundedPrice + (multiple * 4)) {
		strikes.push_back(strikePrice);
		strikePrice += multiple;
	}

	// for (auto i : strikes) std::cout << i << " ";
	// std::cout << std::endl;

	return strikes;
}
