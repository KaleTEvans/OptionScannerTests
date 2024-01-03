#include "DatabaseManager.h"

namespace OptionDB {

	DatabaseManager::DatabaseManager() {
		conn_ = std::make_shared<nanodbc::connection>(connectToDB());
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
		if (dbInsertionThread.joinable()) dbInsertionThread.join();
	}

	void DatabaseManager::addToInsertionQueue(std::shared_ptr<CandleTags> ct) {
		candleProcessingQueue.push(ct);
	}

	void DatabaseManager::addToInsertionQueue(std::shared_ptr<Candle> c, TimeFrame tf) {
		std::unique_lock<std::mutex> lock(queueMtx);
		
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

		while (!stopInsertion) {
			std::unique_lock<std::mutex> lock(queueMtx);
			// wait for data or exit signal
			cv.wait(lock, [this]() {
				return !candleProcessingQueue.empty() || !underlyingQueue.empty() || stopInsertion;
			});

			if (stopInsertion) break;

			lock.unlock();

			while (!candleProcessingQueue.empty() || !underlyingQueue.empty()) {

				if (!candleProcessingQueue.empty()) {
					OptionTable::post(*conn_, candleProcessingQueue.front());
					candleProcessingQueue.pop();
				}

				if (!underlyingQueue.empty()) {
					long unixTime = underlyingQueue.front().first.time_;

					UnderlyingTable::post(*conn_, underlyingQueue.front().first, underlyingQueue.front().second);
					underlyingQueue.pop();

					UnixTable::post(*conn_, unixTime);
				}
			}
		}
	}
}