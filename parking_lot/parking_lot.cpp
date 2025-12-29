
/*
Step 1: Requirements & Constraints
Support: Multiple vehicle types (Bike, Car, Truck) and spot types (Compact, Large, Handicapped).

Multiplicity: 1 Lot has many Levels; 1 Level has many Spots.

Algorithm: Strategy-based spot assignment (e.g., Lowest Floor First) and fee calculation.

Concurrency: Multiple entry/exit gates must not assign the same spot simultaneously.
*/


/*
Step 2: List APIs
parkVehicle(licensePlate, vehicleType) -> Returns TicketID.

unparkVehicle(ticketId) -> Returns Fee.

getAvailableSlots(vehicleType) -> Returns int.
*/



//Step 3
/*
Vehicle 
- licensePlate
- type
ParkingSpot
- id
- type
- isOccupied : bool
ParkingLevel
- id
- levelNumber
- spots : vector<ParkingSpot>
ParkingLot
- id
- name
- levels : vector<ParkingLevel>
Ticket
- id
- vehicleLicensePlate
- parkingSpotId
- levelId
- inTime
- outTime
- fees
*/

//Step 4

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <cmath>
#include <memory>

using namespace std;

// --- Enums & Helpers ---
enum class VehicleType { BIKE, CAR, TRUCK };
enum class SpotType { COMPACT, LARGE, HANDICAPPED };
using TimePoint = chrono::system_clock::time_point;

// ==========================================
// Step 3: Entities
// ==========================================

class ParkingSpot {
    int id;
    SpotType type;
    bool occupied;
public:
    ParkingSpot(int id, SpotType t) : id(id), type(t), occupied(false) {}
    int getId() const { return id; }
    bool isOccupied() const { return occupied; }
    void setOccupied(bool status) { occupied = status; }

    bool canFit(VehicleType vType) const {
        if (type == SpotType::HANDICAPPED) return true; 
        if (vType == VehicleType::BIKE) return true;
        if (vType == VehicleType::CAR) return (type == SpotType::COMPACT || type == SpotType::LARGE);
        if (vType == VehicleType::TRUCK) return (type == SpotType::LARGE);
        return false;
    }
};

class Ticket {
public:
    string id;
    string licensePlate;
    int spotId;
    TimePoint entryTime;
    bool isActive;

    Ticket() : spotId(-1), isActive(false) {}
    Ticket(string id, string lp, int sid) 
        : id(id), licensePlate(lp), spotId(sid), entryTime(chrono::system_clock::now()), isActive(true) {}
};

// ==========================================
// Step 5: Design Patterns (Repositories & Strategies)
// ==========================================

class TicketRepository {
    unordered_map<string, Ticket> ticketDb; // In-memory DB , map TicketID to Ticket
public:
    void save(Ticket t) {
        string id = t.id;
        ticketDb[id] = move(t);
    }
    Ticket* findById(string id) { return ticketDb.count(id) ? &ticketDb[id] : nullptr; }
};

class IAssignmentStrategy {
public:
    virtual ParkingSpot* findSpot(vector<vector<ParkingSpot>>& levels, VehicleType vType) = 0;
};

class LowestFloorFirst : public IAssignmentStrategy {
public:
    ParkingSpot* findSpot(vector<vector<ParkingSpot>>& levels, VehicleType vType) override {
        for (auto& level : levels) {
            for (auto& spot : level) {
                if (!spot.isOccupied() && spot.canFit(vType)) return &spot;
            }
        }
        return nullptr;
    }
};

class IFeeStrategy {
public:
    virtual double calculate(TimePoint entry) = 0;
};

class HourlyFee : public IFeeStrategy {
public:
    double calculate(TimePoint entry) override {
        auto now = chrono::system_clock::now();
        auto duration = chrono::duration_cast<chrono::hours>(now - entry).count();
        return max(1.0, (double)duration + 1) * 10.0; // $10/hr, min 1 hr
    }
};

// ==========================================
// Step 2, 6, 7: Service Flow & Concurrency
// ==========================================



class ParkingLotService {
    vector<vector<ParkingSpot>> levels; 
    TicketRepository ticketRepo;
    IAssignmentStrategy* assignmentStrategy;
    IFeeStrategy* feeStrategy;
    mutex lotMutex; // Step 7: Concurrency control

public:
    ParkingLotService(int numLevels, int spotsPerLevel) {
        for (int i = 0; i < numLevels; i++) {
            vector<ParkingSpot> level;
            for (int j = 0; j < spotsPerLevel; j++) {
                SpotType st = (j % 3 == 0) ? SpotType::LARGE : (j % 2 == 0 ? SpotType::COMPACT : SpotType::HANDICAPPED);
                level.emplace_back(i * 100 + j, st);
            }
            levels.push_back(move(level));
        }
        assignmentStrategy = new LowestFloorFirst();
        feeStrategy = new HourlyFee();
    }

    string parkVehicle(string lp, VehicleType vType) {
        lock_guard<mutex> lock(lotMutex); // Atomic booking

        ParkingSpot* spot = assignmentStrategy->findSpot(levels, vType);
        if (!spot) return "ERROR: Parking Full";

        spot->setOccupied(true);
        string tId = "TKT-" + lp + "-" + to_string(rand() % 1000);
        ticketRepo.save(Ticket(tId, lp, spot->getId()));

        return tId;
    }

    double unparkVehicle(string ticketId) {
        lock_guard<mutex> lock(lotMutex);

        Ticket* t = ticketRepo.findById(ticketId);
        if (!t || !t->isActive) throw runtime_error("Invalid Ticket");

        // Release spot
        for (auto& level : levels) {
            for (auto& spot : level) {
                if (spot.getId() == t->spotId) {
                    spot.setOccupied(false);
                    break;
                }
            }
        }

        t->isActive = false;
        return feeStrategy->calculate(t->entryTime);
    }

    int getAvailableSlots(VehicleType vType) {
        int count = 0;
        for (auto& level : levels) {
            for (auto& spot : level) {
                if (!spot.isOccupied() && spot.canFit(vType)) count++;
            }
        }
        return count;
    }
};

// ==========================================
// Main Function
// ==========================================
int main() {
    ParkingLotService service(2, 10);

    cout << "Initial CAR slots: " << service.getAvailableSlots(VehicleType::CAR) << endl;

    string ticket = service.parkVehicle("ABC-123", VehicleType::CAR);
    cout << "Vehicle Parked. Ticket: " << ticket << endl;

    cout << "CAR slots remaining: " << service.getAvailableSlots(VehicleType::CAR) << endl;

    double fee = service.unparkVehicle(ticket);
    cout << "Vehicle Unparked. Fee Owed: $" << fee << endl;

    return 0;
}