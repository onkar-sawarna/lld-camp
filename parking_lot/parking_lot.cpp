#include<bits/stdc++.h>
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
    bool isOccupied;
};

class ParkingLevel {
    int id;
    int levelNumber;
    vector<ParkingSpot> spots;
    int countAvailableSpots(VehicleType type) const {
        // Logic to count available spots
        return 42; // Example available spots
    }
};

class ParkingLot {
    int id;
    string name;
    vector<ParkingLevel> levels;
    string paekVehicle(string licensePlate, VehicleType type) {
        // Logic to park vehicle
        return "Vehicle parked successfully.";
    }
    double unparkVehicle(string ticketId) {
        // Logic to unpark vehicle and calculate fees
        return 10.0; // Example fee
    }
    int getAvailableSlots(VehicleType type) {
        // Logic to count available slots
        int totalAvailable = 0;
        for (const auto& level : levels) {
            totalAvailable += level.countAvailableSpots(type);
        }
        return 42; // Example available slots
    }
};

class Ticket {
    int id;
    string vehicleLicensePlate;
    int parkingSpotId;
    int levelId;
    TimePoint inTime;
    TimePoint outTime;
    double fees;
};

//Step 2
class ParkingLotService {
    ParkingLot parkingLot;
    public:
    string parkVehicle();
    double unparkVehicle();
    int getAvailableSlots();
};