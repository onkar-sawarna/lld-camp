#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <map>
#include <stdexcept>
#include <cmath> // For std::round

using namespace std;

// --- Step 4 (Class Definitions with added Constructors) ---
using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;

enum class VehicleType {
    CAR,
    BIKE,
    TRUCK
};

string vehicleTypeToString(VehicleType type) {
    switch(type) {
        case VehicleType::CAR: return "CAR";
        case VehicleType::BIKE: return "BIKE";
        case VehicleType::TRUCK: return "TRUCK";
        default: return "UNKNOWN";
    }
}

class Vehicle {
    string licensePlate_;
    VehicleType type_; 
public:
    Vehicle(string lp, VehicleType t) : licensePlate_(lp), type_(t) {}
    string getLicensePlate() const { return licensePlate_; }
    VehicleType getType() const { return type_; }
};

enum class SpotType {
    COMPACT,
    LARGE,
    HANDICAPPED
};

class ParkingSpot {
    int id_;
    SpotType type_;
    bool isOccupied_;
public:
    ParkingSpot(int spotId, SpotType t) : id_(spotId), type_(t), isOccupied_(false) {}
    bool isOccupied() const { return isOccupied_; }
    void setOccupied(bool status) { isOccupied_ = status; }
    // More realistic canFit logic
    bool canFit(VehicleType vehicleType) const {
        if (type_ == SpotType::HANDICAPPED) return true; // Assume any vehicle can use handicapped if needed
        if (vehicleType == VehicleType::CAR && type_ == SpotType::COMPACT) return true;
        if ((vehicleType == VehicleType::CAR || vehicleType == VehicleType::TRUCK) && type_ == SpotType::LARGE) return true;
        if (vehicleType == VehicleType::BIKE && type_ == SpotType::COMPACT) return true; // Bikes often use compact or dedicated areas
        return false;
    }
    void setId(int spotId) { id_ = spotId; }
    int getId() const { return id_; }
    SpotType getType() const { return type_; }
};

class ParkingLevel {
    int id_;
    int levelNumber_;
    vector<ParkingSpot> spots_;
public:
    ParkingLevel(int levelId, int levelNum) : id_(levelId), levelNumber_(levelNum) {}
    void addSpot(const ParkingSpot& spot) { spots_.push_back(spot); }
    int getLevelNumber() const { return levelNumber_; }

    int countAvailableSpots(VehicleType type) const {
        int count = 0;
        for(const auto& spot : spots_) {
            if(!spot.isOccupied() && spot.canFit(type)) {
                count++;
            }
        }
        return count; 
    }
    ParkingSpot* findAvailableSpot(VehicleType type) {
        for(auto& spot : spots_) {
            if(!spot.isOccupied() && spot.canFit(type)) {
                return &spot;
            }
        }
        return nullptr;
    }
    vector<ParkingSpot>& getSpots() {
        return spots_;
    }
};

class ISpotAssignmentStrategy {
public:
    virtual ~ISpotAssignmentStrategy() = default;
    virtual ParkingSpot* assignSpot(vector<ParkingLevel>& levels, VehicleType type) = 0;
};

class LowestLevelFirstStrategy : public ISpotAssignmentStrategy {
public:
    ParkingSpot* assignSpot(vector<ParkingLevel>& levels, VehicleType type) override {
        cout << " [Strategy] Applying Lowest Level First Assignment." << endl;
        for (auto& level : levels) {
            ParkingSpot* spot = level.findAvailableSpot(type);
            if (spot) {
                cout << " [Strategy] Found spot " << spot->getId() << " on Level " << level.getLevelNumber() << "." << endl;
                return spot;
            }
        }
        return nullptr;
    }
};

class IRateCalculationStrategy {
public:
    virtual ~IRateCalculationStrategy() = default;
    virtual double calculateFees(TimePoint inTime, TimePoint outTime) = 0;
};  

class HourlyRateCalculationStrategy : public IRateCalculationStrategy {
public:
    double calculateFees(TimePoint inTime, TimePoint outTime) override {
        // Calculate duration in hours, rounding up to the nearest hour
        auto duration_seconds = chrono::duration_cast<chrono::seconds>(outTime - inTime).count();
        double duration_hours = static_cast<double>(duration_seconds) / 3600.0;
        
        // Round up to the next full hour for billing
        int billed_hours = static_cast<int>(ceil(duration_hours)); 
        
        double ratePerHour = 10.0; // $10 per hour
        double fee = billed_hours * ratePerHour;
        cout << " [Fee Calc] Duration: " << duration_hours << " hours. Billed: " << billed_hours << " hours at $" << ratePerHour << "/hr." << endl;
        return fee;
    }
};  

class ParkingLot {
    int id_;
    string name_;
    vector<ParkingLevel> levels_;
public:
    ParkingLot(int lotId, string lotName) : id_(lotId), name_(lotName) {}
    void addLevel(const ParkingLevel& level) { levels_.push_back(level); }
    vector<ParkingLevel>& getLevels() { return levels_; }
    string getName() const { return name_; }

    // Helper to get spot by ID across all levels (needed for unpark logic)
    ParkingSpot* getSpotById(int spotId) {
        for (auto& level : levels_) {
            for (auto& spot : level.getSpots()) {
                if (spot.getId() == spotId) {
                    return &spot;
                }
            }
        }
        return nullptr;
    }
};

// --- Mock Ticket Implementation ---
class Ticket {
private:
    static int nextId;
    int id_;
    string vehicleLicensePlate_;
    int parkingSpotId_;
    TimePoint inTime_;
    TimePoint outTime_;
    double fees_;

public:
    Ticket(string licensePlate, int spotId, TimePoint inTime) 
        : id_(++nextId), 
          vehicleLicensePlate_(licensePlate), 
          parkingSpotId_(spotId), 
          inTime_(inTime), 
          fees_(0.0) {}

    int getId() const { return id_; }
    int getParkingSpotId() const { return parkingSpotId_; }
    void close(double calculatedFee) {
        outTime_ = Clock::now();
        fees_ = calculatedFee;
    }
    bool closed() const {
        return outTime_.time_since_epoch().count() != 0;
    }
    TimePoint getInTime() const { return inTime_; }
    TimePoint getOutTime() const { return outTime_; }
    double getFees() const { return fees_; }
};
int Ticket::nextId = 0;

class TicketRepository {
private:
    // MOCK: Use a static map to simulate a persistent store for tickets
    static map<string, Ticket*> tickets;
public:
    Ticket* createTicket(string licensePlate, int spotId, TimePoint inTime) {
        Ticket* newTicket = new Ticket(licensePlate, spotId, inTime);
        tickets[to_string(newTicket->getId())] = newTicket;
        return newTicket;
    }
    Ticket* getTicket(string ticketId) {
        if (tickets.count(ticketId)) {
            return tickets.at(ticketId);
        }
        return nullptr;
    }
};
map<string, Ticket*> TicketRepository::tickets; // Initialize the static map

// --- Step 2 (Service Layer) ---
class ParkingLotService {
    ParkingLot& parkingLot_; // Use reference to work with the actual lot
    ISpotAssignmentStrategy* spotAssignmentStrategy_;
    IRateCalculationStrategy* feeCalculationStrategy_;
    TicketRepository ticketRepo_; // Service owns the repository
    mutex lotMutex; 
public:
    ParkingLotService(ParkingLot& parkingLot, ISpotAssignmentStrategy* spotStrategy, IRateCalculationStrategy* feeStrategy)
        : parkingLot_(parkingLot), spotAssignmentStrategy_(spotStrategy), feeCalculationStrategy_(feeStrategy) {}

    string parkVehicle(string licensePlate, VehicleType type) {
        lock_guard<mutex> lock(lotMutex);
        cout << "\n>>> Attempting to park " << vehicleTypeToString(type) << " (" << licensePlate << ")..." << endl;
        
        ParkingSpot* spot = spotAssignmentStrategy_->assignSpot(parkingLot_.getLevels(), type);
        if (!spot) {
            cout << " Parking Failed: No available spots for " << vehicleTypeToString(type) << "." << endl;
            return "";
        }
        
        // 1. Mark the spot as occupied
        spot->setOccupied(true);
        cout << " Parked successfully in Spot ID: " << spot->getId() << endl;
        
        // 2. Generate a ticket
        Ticket* ticket = ticketRepo_.createTicket(
            licensePlate,
            spot->getId(),
            Clock::now()
        );
        
        // 3. Return the ticket ID
        return to_string(ticket->getId());
    }

    double unparkVehicle(string ticketId) {
        lock_guard<mutex> lock(lotMutex);
        cout << "\n<<< Attempting to unpark vehicle with Ticket ID: " << ticketId << "..." << endl;

        // 1. Find the ticket
        Ticket* ticket = ticketRepo_.getTicket(ticketId);
        if (!ticket) {
            throw runtime_error("Ticket not found.");
        }
        if(ticket->closed()) {
            throw runtime_error("Ticket already closed.");
        }
        
        TimePoint now = Clock::now();

        // 2. Calculate parking duration and fees
        double fee = feeCalculationStrategy_->calculateFees(ticket->getInTime(), now);

        // 3. Close the ticket
        ticket->close(fee);
        
        // 4. Mark the spot as available
        ParkingSpot* spot = parkingLot_.getSpotById(ticket->getParkingSpotId());
        if (!spot) {
             // This is a system error if the spot ID is invalid
            cerr << "CRITICAL ERROR: Spot ID " << ticket->getParkingSpotId() << " from ticket " << ticketId << " not found!" << endl;
        } else {
             spot->setOccupied(false);
             cout << " Spot ID " << spot->getId() << " is now marked as FREE." << endl;
        }
        
        cout << " Unparked successfully. Total Fee: $" << fee << endl;
        return fee;
    }

    int getAvailableSlots(VehicleType type) {
        int totalAvailable = 0;
        for (const auto& level : parkingLot_.getLevels()) {
            totalAvailable += level.countAvailableSpots(type);
        }
        return totalAvailable;
    }
    
    // Helper to simulate time passing for fee calculation illustration
    void simulateTimePass(int seconds) {
        cout << " [SIMULATION] " << seconds << " seconds passed..." << endl;
        // In a real system, you'd use a mock clock or injection for testing time.
        // For this simple demo, we just print a message. The `Clock::now()` will capture the actual time difference.
    }
};

// --- Main Function for Illustration ---
int main() {
    cout << "### Parking Lot Simulation Start ###" << endl;
    
    // --- 1. Setup Parking Lot Structure ---
    
    // 1A. Create a few spots for Level 1 (ID: 101, 102, 103)
    ParkingLevel level1(1, 1);
    level1.addSpot(ParkingSpot(101, SpotType::COMPACT)); // Car, Bike
    level1.addSpot(ParkingSpot(102, SpotType::LARGE));   // Car, Truck
    level1.addSpot(ParkingSpot(103, SpotType::COMPACT)); // Car, Bike
    
    // 1B. Create a few spots for Level 2 (ID: 201, 202)
    ParkingLevel level2(2, 2);
    level2.addSpot(ParkingSpot(201, SpotType::HANDICAPPED)); // Any
    level2.addSpot(ParkingSpot(202, SpotType::LARGE));       // Car, Truck

    // 1C. Create the ParkingLot and add levels
    ParkingLot mainLot(10, "Downtown Garage");
    mainLot.addLevel(level1);
    mainLot.addLevel(level2);
    
    // --- 2. Setup Strategies and Service ---
    
    // Using Strategy Pattern
    LowestLevelFirstStrategy spotStrategy;
    HourlyRateCalculationStrategy feeStrategy;
    
    // Initialize the Service
    ParkingLotService lotService(mainLot, &spotStrategy, &feeStrategy);
    
    cout << "--- Parking Lot: " << mainLot.getName() << " Initialized ---" << endl;
    cout << "Available CAR slots: " << lotService.getAvailableSlots(VehicleType::CAR) << endl; // Should be 5
    cout << "Available TRUCK slots: " << lotService.getAvailableSlots(VehicleType::TRUCK) << endl; // Should be 3
    cout << "--------------------------------------------------------" << endl;

    // --- 3. Parking Scenarios ---
    
    // Scenario 1: Park a CAR
    string ticketCar = lotService.parkVehicle("ABC-123", VehicleType::CAR);
    if (!ticketCar.empty()) {
        cout << " CAR Parked. Ticket ID: " << ticketCar << endl; // Should be in Spot 101 (Lowest Level First)
    }

    // Scenario 2: Park a TRUCK
    string ticketTruck = lotService.parkVehicle("XYZ-789", VehicleType::TRUCK);
    if (!ticketTruck.empty()) {
        cout << " TRUCK Parked. Ticket ID: " << ticketTruck << endl; // Should be in Spot 102 (Lowest Level First, and LARGE)
    }
    
    // Scenario 3: Park another CAR (moves to next available on lowest level, then up)
    string ticketCar2 = lotService.parkVehicle("DEF-456", VehicleType::CAR);
    if (!ticketCar2.empty()) {
        cout << " CAR 2 Parked. Ticket ID: " << ticketCar2 << endl; // Should be in Spot 103
    }
    
    // Now, Level 1 Compact (101, 103) and Large (102) are occupied.
    // Next CAR should go to Level 2 Spot 201 (Handicapped) or 202 (Large)
    string ticketCar3 = lotService.parkVehicle("GHI-101", VehicleType::CAR);
    if (!ticketCar3.empty()) {
        cout << " CAR 3 Parked. Ticket ID: " << ticketCar3 << endl; // Should be in Spot 201 or 202
    }
    
    cout << "\nAvailable CAR slots now: " << lotService.getAvailableSlots(VehicleType::CAR) << endl; // Should be 1 remaining

    // --- 4. Unparking Scenario (Fee Calculation) ---
    
    // Simulate time passing before unparking the first car
    lotService.simulateTimePass(3600 + 1); // 1 hour and 1 second

    try {
        double fee = lotService.unparkVehicle(ticketCar);
        cout << " Final CAR (ABC-123) Fee: $" << fee << " (Billed 2 hours due to rounding up)." << endl;
    } catch (const runtime_error& e) {
        cerr << "Unpark Failed: " << e.what() << endl;
    }

    cout << "\nAvailable CAR slots after unparking: " << lotService.getAvailableSlots(VehicleType::CAR) << endl; // Should increase by 1
    
    // --- 5. Illustrating No Available Spot ---
    
    // Occupy all remaining spots to force failure
    lotService.parkVehicle("J-001", VehicleType::CAR); // Takes remaining spot
    lotService.parkVehicle("K-002", VehicleType::TRUCK); // Takes remaining spot

    string ticketFail = lotService.parkVehicle("L-999", VehicleType::BIKE);
    if (ticketFail.empty()) {
        cout << " BIKE Park attempt failed as expected: No available spots." << endl;
    }
    
    cout << "\n### Parking Lot Simulation End ###" << endl;
    
    // Cleanup (in a real app, smart pointers would manage this)
    // Note: Tickets are currently dynamically allocated in the TicketRepository mock.
    // A proper destructor/cleanup would be needed for a robust application.
    return 0;
}