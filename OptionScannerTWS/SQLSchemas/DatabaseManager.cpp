#include "DatabaseManager.h"

namespace OptionDB {

	DatabaseManager::DatabaseManager(bool testConfigNoDB) : testConfigNoDB(testConfigNoDB) {
		if (!testConfigNoDB) conn_ = std::make_shared<nanodbc::connection>(connectToDB());
		OptionDB::resetTables(*conn_);

		OptionDB::UnixTable::setTable(*conn_);
		OptionDB::UnderlyingTable::setTable(*conn_);
		OptionDB::OptionTable::setTable(*conn_);
	}

	void DatabaseManager::start() {
		// Start the db insertion thread
		dbInsertionThread = std::thread([this]() {
			candleInsertionLoop();
		});
	}

	void DatabaseManager::stop() {
		// Signal insertion thread to stop
		stopInsertion = true;

		// Notify the cv in casse the insertion thread is wating
		cv.notify_all();

		if (dbInsertionThread.joinable()) dbInsertionThread.join();

		processingComplete_ = true;
	}

	bool DatabaseManager::processingComplete() const { return processingComplete_; }

	void DatabaseManager::addToInsertionQueue(std::shared_ptr<CandleTags> ct) {
		//std::unique_lock<std::mutex> lock(queueMtx);
		//std::cout << "Received CandleTags" << std::endl;
		candleProcessingQueue.push(ct);
	}

	void DatabaseManager::addToInsertionQueue(std::shared_ptr<Candle> c, TimeFrame tf) {
		//std::unique_lock<std::mutex> lock(queueMtx);
		//std::cout << "Received Underlying" << std::endl;
		
		// Copy data into underlyingQueue for insertion
		UnderlyingTable::CandleForDB candle(
			c->reqId(),
			c->date(),
			c->time(),
			c->open(),
			c->high(),
			c->low(),
			c->close(),
			c->volume()
		);

		underlyingQueue.push({ candle, tf });
	}

	int DatabaseManager::getUnderlyingCount() { return UnderlyingTable::candleCount(*conn_); }
	int DatabaseManager::getOptionCount() { return OptionTable::candleCount(*conn_); }

	void DatabaseManager::setCandleTables() {
		UnixTable::setTable(*conn_);
		UnderlyingTable::setTable(*conn_);
		OptionTable::setTable(*conn_);
	}

	void DatabaseManager::setAlertTables() {
		AlertTables::setTagTable(*conn_);
		AlertTables::setAlertTable(*conn_);
		AlertTables::setTagMappingTable(*conn_);
		AlertTables::setAlertCombinationTable(*conn_);
	}

	void DatabaseManager::candleInsertionLoop() {

		while (true) {
			std::unique_lock<std::mutex> lock(queueMtx);
			// wait for data or exit signal
			cv.wait(lock, [this]() {
				return !candleProcessingQueue.empty() || !underlyingQueue.empty() || stopInsertion;
			});

			// Exit if there is no more data to be processed 
			if (stopInsertion && candleProcessingQueue.empty() && underlyingQueue.empty()) break;

			lock.unlock();

			while (!candleProcessingQueue.empty() || !underlyingQueue.empty()) {

				if (!underlyingQueue.empty()) {
					long unixTime = underlyingQueue.front().first.time_;
					if (timeSet.find(unixTime) == timeSet.end()) {
						timeSet.insert(unixTime);
						if (!testConfigNoDB) UnixTable::post(*conn_, unixTime);
					}

					if (!testConfigNoDB) {
						UnderlyingTable::post(*conn_, underlyingQueue.front().first, underlyingQueue.front().second);
					}
					else {
						UnderlyingTable::CandleForDB cdb = underlyingQueue.front().first;
						TimeFrame tf = underlyingQueue.front().second;
						std::cout << cdb.reqId_ << " | " << tf << " | Underlying" << std::endl;
					}
					underlyingQueue.pop();
				}

				if (!candleProcessingQueue.empty()) {
					long unixTime = candleProcessingQueue.front()->candle.time();
					if (timeSet.find(unixTime) == timeSet.end()) {
						timeSet.insert(unixTime);
						if (!testConfigNoDB) UnixTable::post(*conn_, unixTime);
					}

					if (!testConfigNoDB) {
						OptionTable::post(*conn_, candleProcessingQueue.front());
					}
					else {
						std::shared_ptr<CandleTags> ct = candleProcessingQueue.front();
						std::cout << ct->candle.reqId() << " | " << ct->getTimeFrame() << " | " << ct->getOptType() << " | " << ct->getTOD() << " | " <<
							ct->getRTM() << " | " << ct->getVolStDev() << " | " << ct->getVolThresh() << " | "  << ct->getOptPriceDelta() << " | " <<
							ct->getDHL() << " | " << ct->getLHL() << " | " << ct->getUnderlyingPriceDelta() << " | " << ct->getUnderlyingDHL() 
							<< " | " << ct->getUnderlyingLHL() << std::endl;
					}
					candleProcessingQueue.pop();
				}
			}

			// Re-check stop-insertion after processing
			if (stopInsertion) {
				std::unique_lock<std::mutex> relock(queueMtx);
				if (candleProcessingQueue.empty() && underlyingQueue.empty()) break;
			}
		}

	}

	std::mutex& DatabaseManager::getMtx() { return queueMtx; }
	std::condition_variable& DatabaseManager::getCV() { return cv; }
}