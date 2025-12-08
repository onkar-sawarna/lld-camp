// 03-notify-dip-ocp.cpp
#include <iostream>
#include <string>

using namespace std;

class SmtpMailer {
public:
    void send(const string& templ, const string& to, const string& body){
        cout << "[SMTP] template=" << templ << " to=" << to << " body=" << body << "\n";
    }
};
class TwilioClient {
public:
    void sendOTP(const string& phone, const string& code){
        cout << "[Twilio] OTP " << code << " -> " << phone << "\n";
    }
};

struct User { string email; string phone; };

class SignUpService {
public:
    bool signUp(const User& u){
        if (u.email.empty()) return false;
        // pretend DB save here…

        // hard-coded providers
        SmtpMailer mailer;
        mailer.send("welcome", u.email, "Welcome!");

        TwilioClient sms;
        sms.sendOTP(u.phone, "123456");
        return true;
    }
};

int main() {
    SignUpService svc;
    svc.signUp({"user@example.com", "+15550001111"});
    return 0;
}
