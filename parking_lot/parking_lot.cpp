#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <mutex>


using namespace std;

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
using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;

enum class VehicleType {
    CAR,
    BIKE,
    TRUCK
};
class Vehicle {
    string licensePlate;
    VehicleType type; // e.g., "Car", "Bike", "Truck"
};
enum class SpotType {
    COMPACT,
    LARGE,
    HANDICAPPED
};
class ParkingSpot {
    int id;
    SpotType type; // e.g., "Compact", "Large", "Handicapped"
    bool isOccupied_;
public:
    ParkingSpot(int spotId, SpotType spotType) : id(spotId), type(spotType), isOccupied_(false) {}
    bool isOccupied() const { return isOccupied_; }
    void setOccupied(bool status) { isOccupied_ = status; }
    bool canFit(VehicleType vehicleType) const {
        // Logic to determine if the spot can fit the vehicle type
        return true; // Placeholder
    }
    void setId(int spotId) { id = spotId; }
    int getId() const { return id; }
};

class ParkingLevel {
    int id;
    int levelNumber;
    vector<ParkingSpot> spots;
public:
    ParkingLevel(int levelId, int levelNum) : id(levelId), levelNumber(levelNum) {}
    void addSpot(const ParkingSpot& spot) { spots.push_back(spot); }
    int countAvailableSpots(VehicleType type) const {
        // Logic to count available spots
        int count = 0;
        for(auto spot : spots) {
            if(!spot.isOccupied() and spot.canFit(type)) {
                count++;
            }
        }
        return count; 
    }
    ParkingSpot* findAvailableSpot(VehicleType type) {
        for(auto& spot : spots) {
            if(!spot.isOccupied() and spot.canFit(type)) {
                return &spot;
            }
        }
        return nullptr;
    }
    vector<ParkingSpot> getSpots() {
        return spots;
    }

};

//Applying Strategy Pattern on spot assignment
class ISpotAssignmentStrategy {
public:
    virtual ~ISpotAssignmentStrategy() = default;
    virtual ParkingSpot* assignSpot(vector<ParkingLevel>& levels, VehicleType type) = 0;
};

class LowestLevelFirstStrategy : public ISpotAssignmentStrategy {
public:
    ParkingSpot* assignSpot(vector<ParkingLevel>& levels, VehicleType type) override {
        for (auto& level : levels) {
            ParkingSpot* spot = level.findAvailableSpot(type);
            if (spot) {
                return spot;
            }
        }
        return nullptr;
    }
};

// Applying strategy pattern on Payment Calculation can be done similarly
class IRateCalculationStrategy {
public:
    virtual ~IRateCalculationStrategy() = default;
    virtual double calculateFees(TimePoint inTime, TimePoint outTime) = 0;
};  
class HourlyRateCalculationStrategy : public IRateCalculationStrategy {
public:
    double calculateFees(TimePoint inTime, TimePoint outTime) override {
        // Logic to calculate hourly fees
        auto duration = chrono::duration_cast<chrono::hours>(outTime - inTime).count();
        double ratePerHour = 10.0; // 
        return duration * ratePerHour;
    }
};  

class ParkingLot {
    int id;
    string name;
    vector<ParkingLevel> levels_;
public:
    ParkingLot() {}
    ParkingLot(int lotId, string lotName) : id(lotId), name(lotName) {}
    void addLevel(const ParkingLevel& level) { levels_.push_back(level); }
    string getName() const { return name; }
    vector<ParkingLevel>& getLevels() {
        return levels_;
    }
};

class TicketRepository {
    public:
    // Implementation for storing and retrieving tickets
    Ticket* createTicket(string licensePlate, int spotId, TimePoint inTime) {
        // Logic to create and store a ticket
        return new Ticket(); // Placeholder
    }
    Ticket* getTicket(string ticketId) {
        // Logic to retrieve a ticket by ID
        return new Ticket(); // Placeholder
    }
};
class Ticket {
    int id;
    string vehicleLicensePlate;
    int parkingSpotId;
    TimePoint inTime;
    TimePoint outTime;
    double fees;
public:
    int getId() const { return id; }
    void close() {
        outTime = Clock::now();
        // Logic to calculate fees
    }
    bool closed() const {
        return outTime.time_since_epoch().count() != 0;
    }
    TimePoint getInTime() const {
        return inTime;
    }
    TimePoint getOutTime() const {
        return outTime;
    }
};

//Step 2
class ParkingLotService {
    ParkingLot parkingLot_;
    ISpotAssignmentStrategy* spotAssignmentStrategy_;
    IRateCalculationStrategy* feeCalculationStrategy_;
    mutex lotMutex; // To handle concurrent access
    public:
    void simulateTimePass(int seconds) {
        this_thread::sleep_for(chrono::seconds(seconds));
    }
    ParkingLotService(ParkingLot parkingLot, ISpotAssignmentStrategy* spotStrategy, IRateCalculationStrategy* feeStrategy) {
        parkingLot_ = parkingLot;
        spotAssignmentStrategy_ = spotStrategy;
        feeCalculationStrategy_ = feeStrategy;
    }
    string parkVehicle(string licensePlate, VehicleType type) {
        lock_guard<mutex> lock(lotMutex);
        // Logic to park vehicle
        // Find an available spot
        ParkingSpot* spot = spotAssignmentStrategy_->assignSpot(parkingLot_.getLevels(), type);
        if (!spot) {
            return "No available spots.";
        }
        // Mark the spot as occupied
        spot->setOccupied(true);
        // Generate a ticket
        TicketRepository ticketRepo;
        Ticket* ticket = ticketRepo.createTicket(
            licensePlate,
            spot->getId(),
            Clock::now()
        );
        spot->setOccupied(true);
        // Return the ticket ID
        return to_string(ticket->getId());
    }
    double unparkVehicle(string ticketId) {
        lock_guard<mutex> lock(lotMutex);
        // Logic to unpark vehicle and calculate fees
        // Find the ticket
        TicketRepository ticketRepo;
        Ticket* ticket = ticketRepo.getTicket(ticketId);
        if (!ticket) {
            throw runtime_error("Ticket not found"); // Ticket not found
        }
        
        if(ticket->closed()) {
            throw runtime_error("Ticket already closed");
        }
        double fee = feeCalculationStrategy_->calculateFees(ticket->getInTime(),Clock::now());
        ticket->close();
        // Mark the spot as available
        for(auto& level : parkingLot_.getLevels()) {
            for(auto& spot : level.getSpots()) {
                if(spot.getId() == ticket->getId()) {
                    spot.setOccupied(false);
                }
            }
        }
        // Calculate parking duration and fees  
        return fee;
    }
    int getAvailableSlots(VehicleType type) {
        // Logic to count available slots
        int totalAvailable = 0;
        for (const auto& level : parkingLot_.getLevels()) {
            totalAvailable += level.countAvailableSpots(type);
        }
        return totalAvailable;
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