#include "AlertHandler.h"

namespace Alerts {

	Alert::Alert(std::shared_ptr<CandleTags> candle) : ct(candle) {

		initTime = std::chrono::steady_clock::now();
	}

	//===================================================
	// Alert Handler
	//===================================================

	AlertHandler::AlertHandler(std::shared_ptr<std::unordered_map<int, std::shared_ptr<ContractData>>> contractMap) :
		contractMap_(contractMap) {

		// Start a thread to check the alerts
		alertCheckThread_ = std::thread(&AlertHandler::checkAlertOutcomes, this);
	}

	void AlertHandler::inputAlert(std::shared_ptr<CandleTags> candle) {
		Alert alert(candle);
		alertUpdateQueue.push(alert);
	}

	void AlertHandler::checkAlertOutcomes() {
		// Start time to reference and log win rate data every 30 minutes
		std::chrono::steady_clock::time_point refTime = std::chrono::steady_clock::now();

		while (!doneCheckingAlerts_) {
			while (!alertUpdateQueue.empty()) {
				// Check current time to see if 30 minutes have passed since the first alert was added to the queue
				std::chrono::steady_clock::time_point prevAlertTIme = alertUpdateQueue.front().initTime;
				std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
				std::chrono::minutes elsapsedTime = std::chrono::duration_cast<std::chrono::minutes>(currentTime - prevAlertTIme);

				if (elsapsedTime >= std::chrono::minutes(30)) {
					std::unique_lock<std::mutex> lock(alertMtx_);

					Alert a = alertUpdateQueue.front();
					// Access data from the contract map
					std::vector<std::shared_ptr<Candle>> prevCandles = contractMap_->at(a.ct->candle.reqId())->candlesLast30Minutes();

					// just use the lock to access the contract map
					lock.unlock();

					// Retrieve pair of win bool and percent win
					std::pair<double, double> winStats = checkWinStats(prevCandles, a);

					alertUpdateQueue.pop();
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
	}

	void AlertHandler::doneCheckingAlerts() {
		doneCheckingAlerts_ = true;
		alertCheckThread_.join();
	}

	//========================================================
	// Helper Functions
	//========================================================

	std::pair<double, double> checkWinStats(std::vector<std::shared_ptr<Candle>> prevCandles, Alert a) {
		// Ensure we start at a vector candle that is after the alert time
		// If we are close to the market close, it won't be a full 30 minutes
		size_t i = 0;
		while (a.ct->candle.time() > prevCandles[i]->time()) i++;

		double startPrice = a.ct->candle.close();
		double maxPrice = startPrice;
		double minPrice = startPrice;
		double percentChangeHigh = 0;
		double percentChangeLow = 0;

		for (i; i < prevCandles.size(); i++) {

			// If percentChangeLow gets to -30% break and compare to percentChangeHigh
			if (percentChangeLow <= -30.0) {
				break;
			}

			minPrice = min(minPrice, prevCandles[i]->low());
			maxPrice = max(maxPrice, prevCandles[i]->high());

			percentChangeLow = ((minPrice - startPrice) / startPrice) * 100;
			percentChangeHigh = ((maxPrice - startPrice) / startPrice) * 100;
		}

		if (percentChangeHigh <= 0) {
			return { 0, 0 };
		}
		else  if (percentChangeHigh > 0 && percentChangeHigh < 60.0) {
			return { 0.5, percentChangeHigh };
		}
		else {
			return { 1, percentChangeHigh };
		}
	}
}