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
		//cv.notify_all();

		if (dbInsertionThread.joinable()) dbInsertionThread.join();

		processingComplete_ = true;
	}

	bool DatabaseManager::processingComplete() const { return processingComplete_; }

	void DatabaseManager::addToInsertionQueue(std::shared_ptr<CandleTags> ct) {
		//std::cout << "Incoming Candle Time: " << ct->candle.time() << std::endl;
		//candleQueue[ct->candle.time()].push(ct);
		candlePriorityQueue.push(ct);
	}

	void DatabaseManager::addToInsertionQueue(std::shared_ptr<Candle> c, TimeFrame tf) {
		std::unique_lock<std::mutex> lock(queueMtx);
		std::cout << "Received Underlying Time" << c->time() << std::endl;
		
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

			while (!underlyingQueue.empty() && !candlePriorityQueue.empty()) {
				static long prevUnixTime = -1;
				static bool unixTimeUpdated = false;

				std::vector<std::shared_ptr<CandleTags>> optionBatch;

				if (!underlyingQueue.empty()) {
					std::cout << "Current Underlying Queue Size: " << underlyingQueue.size() << std::endl;
					long unixTime = underlyingQueue.front().first.time_;
					std::cout << "Current Unix Time: " << unixTime << std::endl;

					std::unique_lock<std::mutex> lock(queueMtx);
					prevUnixTime = OptionDB::UnixTable::post(*conn_, unixTime);
					unixTimeUpdated = true;
					lock.unlock();

					OptionDB::UnderlyingTable::post(*conn_, underlyingQueue.front().first, underlyingQueue.front().second);
					underlyingQueue.pop();
				}

				// Give time for insertion to complete
				// std::this_thread::sleep_for(std::chrono::milliseconds(1000));

				if (!candlePriorityQueue.empty() && unixTimeUpdated) {
					while (!candlePriorityQueue.empty() && candlePriorityQueue.top()->candle.time() <= prevUnixTime) {
						std::shared_ptr<CandleTags> ct = candlePriorityQueue.top();
						optionBatch.push_back(ct);
						candlePriorityQueue.pop();
					}

					unixTimeUpdated = false;
				}

				if (optionBatch.size() > 0) OptionDB::OptionTable::post(*conn_, optionBatch);
				optionBatch.clear();
			}

			// Re-check stop-insertion after processing
			if (stopInsertion) {
				if (underlyingQueue.empty() && candlePriorityQueue.empty()) break;
			}
		}

	}

	std::mutex& DatabaseManager::getMtx() { return queueMtx; }
	std::condition_variable& DatabaseManager::getCV() { return cv; }
}