// mini_lms.cpp

#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <memory>

using namespace std;


class Course;
class Module;
class Lesson;
class Enrollment;
class ICourseRepository;
class IEnrollmentRepository;


// =========================
// Repository Interfaces
// =========================
class ICourseRepository {
public:
    virtual ~ICourseRepository() = default;
    virtual Course* findById(int id) = 0;
    virtual vector<Course> findByInstructor(int instructorId) const = 0;
    virtual void save(const Course& course) = 0;
};

class IEnrollmentRepository {
public:
    virtual ~IEnrollmentRepository() = default;
    virtual vector<Enrollment> findByStudent(int studentId) const = 0;
    virtual Enrollment* findByStudentAndCourse(int studentId, int courseId) = 0;
    virtual void save(const Enrollment& enrollment) = 0;
};

// =========================
// User Base Classes
// =========================
class User {
private:
    int id;
    string name;
    string email;

public:
    User(int uid, const string& uname, const string& uemail)
        : id(uid), name(uname), email(uemail) {}

    int getId() const { return id; }
    string getName() const { return name; }
    string getEmail() const { return email; }
};

class Student : public User {
public:
    Student(int id, const string& name, const string& email)
        : User(id, name, email) {}

    vector<Enrollment> getEnrollments(const IEnrollmentRepository& repo) const {
        return repo.findByStudent(getId());
    }
};

class Instructor : public User {
public:
    Instructor(int id, const string& name, const string& email)
        : User(id, name, email) {}

    vector<Course> getCourses(const ICourseRepository& repo) const {
        return repo.findByInstructor(getId());
    }
};


// =========================
// Lesson
// =========================
class Lesson {
private:
    int id;
    string title;
    string contentURL;
    int duration;

public:
    Lesson(int lid, const string& t, const string& url, int d)
        : id(lid), title(t), contentURL(url), duration(d) {}

    int getId() const { return id; }
    string getTitle() const { return title; }
};


// =========================
// Module
// =========================
class Module {
private:
    int id;
    string title;
    vector<Lesson> lessons;

public:
    Module(int mid, const string& t) : id(mid), title(t) {}

    void addLesson(const Lesson& lesson) {
        lessons.push_back(lesson);
        cout << "Lesson " << lesson.getTitle()
             << " added to module " << title << ".\n";
    }

    const vector<Lesson>& getLessons() const { return lessons; }
    string getTitle() const { return title; }
    int getId() const { return id; }
};


// =========================
// Course
// =========================
class Course {
private:
    int instructorId;
    int courseId;
    string title;
    string description;
    vector<Module> modules;

public:
    Course(int id, int instrId, const string& t, const string& desc)
        : courseId(id), instructorId(instrId), title(t), description(desc) {}

    string getTitle() const { return title; }
    int getCourseId() const { return courseId; }

    void addModule(const Module& module) {
        modules.push_back(module);
        cout << "Module added to course " << title << ".\n";
    }

    const vector<Module>& getModules() const {
        return modules;
    }
    int getInstructorId() const { return instructorId; }

    int getTotalLessonCount() const {
        return accumulate(modules.begin(), modules.end(), 0,
            [](int sum, const Module& m) {
                return sum + m.getLessons().size();
            }
        );
    }
};


// =========================
// Enrollment
// =========================
class Enrollment {
public:
    enum class Status {
        IN_PROGRESS,
        COMPLETED
    };

private:
    static int nextId;
    int id;
    int studentId;
    int courseId;
    int enrollmentDate;
    Status status;
    float progressPercent;
    vector<int> completedLessonIds;

public:
    Enrollment(int sId, int cId, int edate)
        : id(nextId++), studentId(sId), courseId(cId), enrollmentDate(edate),
          status(Status::IN_PROGRESS), progressPercent(0.0f) {}

    int getId() const { return id; }
    int getStudentId() const { return studentId; }
    int getCourseId() const { return courseId; }
    Status getStatus() const { return status; }

    void markLessonCompleted(int lessonId) {
        completedLessonIds.push_back(lessonId);
        cout << "Lesson " << lessonId << " marked as completed.\n";
    }

    void updateProgress(int totalLessons) {
        if (totalLessons > 0) {
            progressPercent = (static_cast<float>(completedLessonIds.size()) / totalLessons) * 100.0f;
        }
        if (progressPercent >= 100.0f) {
            status = Status::COMPLETED;
        }
    }

    float getProgressPercent() const {
        return progressPercent;
    }
};

int Enrollment::nextId = 1;


// =========================
// Notification Interface
// =========================
class INotificationChannel {
public:
    virtual ~INotificationChannel() = default;
    virtual void send(const User& user, const string& message) = 0;
};

class EmailNotificationChannel : public INotificationChannel {
public:
    void send(const User& user, const string& message) override {
        cout << "Sending Email to " << user.getEmail()
             << ": " << message << "\n";
    }
};

class SMSNotificationChannel : public INotificationChannel {
public:
    void send(const User& user, const string& message) override {
        cout << "Sending SMS to " << user.getName()
             << ": " << message << "\n";
    }
};


// =========================
// In-Memory Repositories
// =========================
class InMemoryCourseRepo : public ICourseRepository {
private:
    vector<Course> courses;

public:
    Course* findById(int id) override {
        for (auto& c : courses)
            if (c.getCourseId() == id) return &c;
        return nullptr;
    }

    vector<Course> findByInstructor(int instructorId) const override {
        vector<Course> result;
        for (const auto& c : courses) {
            if (c.getInstructorId() == instructorId) {
                result.push_back(c);
            }
        }
        return result;
    }
    void save(const Course& course) override {
        courses.push_back(course);
    }
};

class InMemoryEnrollmentRepo : public IEnrollmentRepository {
private:
    vector<Enrollment> enrollments;

public:
    Enrollment* findByStudentAndCourse(int studentId, int courseId) override {
        for (auto& e : enrollments) {
            if (e.getStudentId() == studentId && e.getCourseId() == courseId) {
                return &e;
            }
        }
        return nullptr;
    }

    vector<Enrollment> findByStudent(int studentId) const override {
        vector<Enrollment> result;
        for (const auto& e : enrollments) {
            if (e.getStudentId() == studentId) {
                result.push_back(e);
            }
        }
        return result;
    }

    void save(const Enrollment& enrollment) override {
        enrollments.push_back(enrollment);
    }
};


// =========================
// Enrollment Service
// =========================
class EnrollmentService {
private:
    ICourseRepository* cRepo;
    IEnrollmentRepository* eRepo;
    INotificationChannel* notifier;

public:
    EnrollmentService(ICourseRepository* cr,
                      IEnrollmentRepository* er,
                      INotificationChannel* n)
        : cRepo(cr), eRepo(er), notifier(n) {}

    Enrollment enroll(Student& student, Course& course) {
        cout << "Enrolling student " << student.getName()
             << " to course " << course.getTitle() << ".\n";

        Enrollment e(student.getId(), course.getCourseId(), 20240601);
        eRepo->save(e);

        notifier->send(student,
                       "You have been enrolled in " + course.getTitle());

        return e;
    }

    void completeLesson(int studentId, int courseId, int lessonId) {
        Course* course = cRepo->findById(courseId);
        if (!course) {
            cout << "Error: Course not found.\n";
            return;
        }

        Enrollment* enrollment = eRepo->findByStudentAndCourse(studentId, courseId);
        if (!enrollment) {
            cout << "Error: Enrollment not found.\n";
            return;
        }

        enrollment->markLessonCompleted(lessonId);
        enrollment->updateProgress(course->getTotalLessonCount());

        cout << "Progress for " << course->getTitle() << ": "
             << enrollment->getProgressPercent() << "%\n";

        if (enrollment->getStatus() == Enrollment::Status::COMPLETED) {
            User* studentUser = new Student(studentId, "", ""); // Mock user for notification
            notifier->send(*studentUser,
                           "Congratulations! You have completed the course: " + course->getTitle());
            delete studentUser;
        }
    }
};

// =========================
// LMS Service (New)
// =========================
class LmsService {
private:
    ICourseRepository* cRepo;
    IEnrollmentRepository* eRepo;

public:
    LmsService(ICourseRepository* cr, IEnrollmentRepository* er)
        : cRepo(cr), eRepo(er) {}

    vector<Course> getCoursesForInstructor(const Instructor& instructor) {
        return instructor.getCourses(*cRepo);
    }

    vector<Enrollment> getEnrollmentsForStudent(const Student& student) {
        return student.getEnrollments(*eRepo);
    }
};


// =========================
// MAIN
// =========================
int main() {
    Student student(1, "Alice", "alice@example.com");
    Instructor instructor(2, "Bob", "bob@example.com");

    // Setup course with modules and lessons
    Course course(101, instructor.getId(), "C++ Basics", "Learn the basics of C++ programming.");
    Module m1(201, "Introduction");
    m1.addLesson(Lesson(301, "History of C++", "url1", 10));
    m1.addLesson(Lesson(302, "Setup Environment", "url2", 20));
    course.addModule(m1);

    Module m2(202, "Core Concepts");
    m2.addLesson(Lesson(303, "Variables and Types", "url3", 15));
    m2.addLesson(Lesson(304, "Control Flow", "url4", 25));
    course.addModule(m2);

    // Setup dependencies
    InMemoryCourseRepo cRepo;
    cRepo.save(course);
    InMemoryEnrollmentRepo eRepo;
    EmailNotificationChannel email;

    // Setup service
    EnrollmentService service(&cRepo, &eRepo, &email);

    // --- Demo Flow ---

    // 1. Enroll student
    service.enroll(student, course);
    cout << "\n--- Starting Lessons ---\n";

    // 2. Complete all lessons
    service.completeLesson(student.getId(), course.getCourseId(), 301);
    service.completeLesson(student.getId(), course.getCourseId(), 302);
    service.completeLesson(student.getId(), course.getCourseId(), 303);
    service.completeLesson(student.getId(), course.getCourseId(), 304);

    cout << "\n--- Final Status ---\n";
    Enrollment* finalEnrollment = eRepo.findByStudentAndCourse(student.getId(), course.getCourseId());
    cout << "Student " << student.getName() << " final progress: " << finalEnrollment->getProgressPercent() << "%\n";

    cout << "\n--- Instructor and Student Data ---\n";
    LmsService lmsService(&cRepo, &eRepo);

    // Get courses for instructor
    vector<Course> instructorCourses = lmsService.getCoursesForInstructor(instructor);
    cout << "Courses taught by " << instructor.getName() << ":\n";
    for (const auto& c : instructorCourses) {
        cout << "- " << c.getTitle() << "\n";
    }

    // Get enrollments for student
    vector<Enrollment> studentEnrollments = lmsService.getEnrollmentsForStudent(student);
    cout << "Enrollments for " << student.getName() << ":\n";
    cout << "- Enrolled in " << studentEnrollments.size() << " course(s).\n";

    return 0;
}
