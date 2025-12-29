//Step 1:
/* 
 
 - type support (filtering logic) : movies (language, genre), seats (silver, gold, platinum)
 - multiplicity : city -> theaters, show -> showseats
 - algorithm / logic : strategy pattern for pricing
 - concurrency (locking mechanism) : during seat selection and cancellation
*/

//Step 2: List APIs
/*
    searchMovies(){}
    listShowForMovie(){}
    getSeatLayoutForShow(){}
    createBooking(){}
    cancelBooking(){}
*/

//Step 3: entities
/*
   City 
    - id
    - name

   Theater
    - id
    - name
    - cityId
    - address

   Screen
    - id
    - theaterId
    - name
    - totalSeats

   Movie
    - id
    - name
    - duration
    - genre
    - language

   Show
    - id
    - screenId
    - movieId
    - startTime
    - price

   Seat
    - id
    - screenId
    - rowNumber
    - seatNumber

   ShowSeat
    - id
    - showId
    - seatId
    - status
    - lockExpiryTime 

   Booking
    - id
    - seatIds
    - showId
    - paymentStatus
    - totalAmount
    - status
*/

// Step 4: class design 

// step 5 : apply design patterns where applicable

// step 6 : sequence for main flow 

// step 7 : concurrency handling where applicable

// ----------------------------------------------------------------------------------------------

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <stdexcept>
#include <memory>

using namespace std;

// =========================================================
// Step 1: Requirements & Constraints
// =========================================================
/* - Type Support: Movies (Languages, Genres), Seats (Silver, Gold, Platinum).
 - Multiplicity: 1 City -> Many Theaters; 1 Show -> Many ShowSeats.
 - Algorithm: Strategy-based Pricing; Filtering logic in Repositories.
 - Concurrency: Thread-safe locking during seat selection and cancellation.
*/

// --- Core Enums & Helper Structs ---
enum class SeatStatus { AVAILABLE, LOCKED, BOOKED };
enum class BookingStatus { PENDING, CONFIRMED, CANCELLED };
struct Date { int day, month, year; };

// =========================================================
// Step 3: Entities
// =========================================================

class Movie {
public:
    int id;
    string title;
    string language;
    Movie(int id, string t, string l) : id(id), title(t), language(l) {}
};

class ShowSeat {
public:
    int seatId;
    double price;
    SeatStatus status;

    ShowSeat() : seatId(0), price(0.0), status(SeatStatus::AVAILABLE) {}
    ShowSeat(int id, double p) : seatId(id), price(p), status(SeatStatus::AVAILABLE) {}
};

class Show {
public:
    int id;
    int movieId;
    int theaterId;
    string startTime;
    unordered_map<int, ShowSeat> seats; // Map seatId to seat instance
};

class Booking {
public:
    int id;
    int userId;
    int showId;
    vector<int> seatIds;
    double amount;
    BookingStatus status;
};

// =========================================================
// Step 4 & 5: Repositories & Design Patterns
// =========================================================

// --- Repository Pattern: Decoupling Data Logic ---
class MovieRepository {
    unordered_map<int, vector<Movie>> cityMovieMap;  // Map cityId to Movies
public:
    vector<Movie> findAllMovies(int cityId, Date date) {
        // Logic to fetch movies active in a city on a specific date
        return cityMovieMap[cityId];
    }
    void addMovieToCity(int cityId, Movie m) { cityMovieMap[cityId].push_back(m); }
};

class ShowRepository {
    unordered_map<int, Show*> showDb; // In-memory DB , map ShowID to Show
public:
    Show* findById(int id) { return showDb.count(id) ? showDb[id] : nullptr; }
    void save(Show* s) { showDb[s->id] = s; }
};

class BookingRepository {
    unordered_map<int, Booking*> bookingDb; // In-memory DB , map BookingID to Booking
    int nextId = 1000;
public:
    void save(Booking* b) { 
        if(b->id == 0) b->id = nextId++;
        bookingDb[b->id] = b; 
    }
    Booking* findById(int id) { return bookingDb.count(id) ? bookingDb[id] : nullptr; }
};

// --- Strategy Pattern: Pricing Logic ---
class IPricingStrategy {
public:
    virtual ~IPricingStrategy() = default;
    virtual double calculate(double base) = 0;
};

class HolidayPricing : public IPricingStrategy {
public:
    double calculate(double base) override { return base * 1.5; } // 50% surge
};

class RegularPricing : public IPricingStrategy {
public:
    double calculate(double base) override { return base; } // No change
};

// =========================================================
// Step 2, 6 & 7: APIs, Sequence Flow & Concurrency
// =========================================================



class BookMyShowService {
    MovieRepository& movieRepo;
    ShowRepository& showRepo;
    BookingRepository& bookingRepo;
    IPricingStrategy* pricingStrategy;
    mutex systemMutex; // Global lock for transactional integrity

public:
    BookMyShowService(MovieRepository& mr, ShowRepository& sr, BookingRepository& br, IPricingStrategy* ps)
        : movieRepo(mr), showRepo(sr), bookingRepo(br), pricingStrategy(ps) {}

    // API: Search
    vector<Movie> searchMovies(int cityId, Date date) {
        return movieRepo.findAllMovies(cityId, date);
    }

    // API: Create Booking (Step 7: Concurrency Handling)
    Booking* createBooking(int userId, int showId, vector<int> seatIds) {
        lock_guard<mutex> lock(systemMutex); // Critical Section start

        Show* show = showRepo.findById(showId);
        if (!show) throw runtime_error("Show not found.");

        // 1. Validate Availability
        for (int sid : seatIds) {
            if (show->seats[sid].status != SeatStatus::AVAILABLE) {
                throw runtime_error("Seat " + to_string(sid) + " is already occupied.");
            }
        }

        // 2. Lock & Calculate Price
        double total = 0;
        for (int sid : seatIds) {
            show->seats[sid].status = SeatStatus::BOOKED;
            total += pricingStrategy->calculate(show->seats[sid].price);
        }

        // 3. Persist Booking
        Booking* b = new Booking{0, userId, showId, seatIds, total, BookingStatus::CONFIRMED};
        bookingRepo.save(b);
        
        cout << "[SUCCESS] Booking " << b->id << " confirmed for $" << total << endl;
        return b;
    }

    // API: Cancel Booking
    bool cancelBooking(int bookingId) {
        lock_guard<mutex> lock(systemMutex);

        Booking* booking = bookingRepo.findById(bookingId);
        if (!booking || booking->status == BookingStatus::CANCELLED) return false;

        Show* show = showRepo.findById(booking->showId);
        if (show) {
            // Release seats back to inventory
            for (int sid : booking->seatIds) {
                show->seats[sid].status = SeatStatus::AVAILABLE;
            }
        }

        booking->status = BookingStatus::CANCELLED;
        cout << "[CANCELLED] Booking " << bookingId << ". Seats released." << endl;
        return true;
    }
};

// =========================================================
// Main Flow Illustration
// =========================================================

int main() {
    // 1. Initialize Infrastructure
    MovieRepository movieRepo;
    ShowRepository showRepo;
    BookingRepository bookingRepo;
    RegularPricing regularSurge;
    HolidayPricing holidaySurge;

    // 2. Mock Data Setup
    movieRepo.addMovieToCity(1, Movie(1, "Oppenheimer", "English"));
    
    Show* s1 = new Show();
    s1->id = 501;
    s1->seats[10] = ShowSeat(10, 20.0); // Seat 10, $20
    s1->seats[11] = ShowSeat(11, 20.0); // Seat 11, $20
    showRepo.save(s1);

    // 3. Initialize Service
    BookMyShowService bms(movieRepo, showRepo, bookingRepo, &holidaySurge);

    // 4. User Scenario
    try {
        cout << "--- User 1 Booking ---" << endl;
        Booking* b1 = bms.createBooking(99, 501, {10, 11});

        cout << "\n--- User 2 Attempting same seats (Should Fail) ---" << endl;
        bms.createBooking(88, 501, {10});

    } catch (const exception& e) {
        cout << "System Message: " << e.what() << endl;
    }

    // 5. Cancellation Scenario
    cout << "\n--- Cancelling User 1's Booking ---" << endl;
    bms.cancelBooking(1000);

    return 0;
}