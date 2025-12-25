// 03-notify-ans.cpp
#include <iostream>
#include <string>

using namespace std;
class ISmtpMailer {
    public:
    virtual ~ISmtpMailer() = default;
    virtual void send(const string& templ, const string& to, const string& body) = 0;
};
class ITwilioClient {
    public:
    virtual ~ITwilioClient() = default;
    virtual void sendOTP(const string& phone, const string& code) = 0; 
};
class SmtpMailer: public ISmtpMailer {
public:
    void send(const string& templ, const string& to, const string& body)override{
        cout << "[SMTP] template=" << templ << " to=" << to << " body=" << body << "\n";
    }
};
class TwilioClient : public ITwilioClient {
public:
    void sendOTP(const string& phone, const string& code)override{
        cout << "[Twilio] OTP " << code << " -> " << phone << "\n";
    }
};

struct User { string email; string phone; };

class SignUpService {
    ISmtpMailer& mailer;
    ITwilioClient& smsClient;

public:
    SignUpService(ISmtpMailer& mailer, ITwilioClient& smsClient)
        : mailer(mailer), smsClient(smsClient) {}

    bool signUp(const User& u){
        if (u.email.empty()) return false;
        // pretend DB save here…

        mailer.send("welcome", u.email, "Welcome!");
        smsClient.sendOTP(u.phone, "123456");
        return true;
    }
};

int main() {
    SmtpMailer mailer;
    TwilioClient smsClient;
    SignUpService svc(mailer, smsClient);
    svc.signUp({"user@example.com", "+15550001111"});
    return 0;
}
