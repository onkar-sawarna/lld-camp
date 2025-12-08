// 01-invoice-srp-ocp.cpp
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

using namespace std;

struct LineItem {
    string sku;
    int quantity{0};
    double unitPrice{0.0};
};

class InvoiceService {
public:
    string process(const vector<LineItem>& items,
                   const map<string, double>& discounts,
                   const string& email) {
        // pricing
        double subtotal = 0.0;
        for (auto& it : items) subtotal += it.unitPrice * it.quantity;

        // discounts (tightly coupled)
        double discount_total = 0.0;
        for (auto& kv : discounts) {
            const string& k = kv.first;
            double v = kv.second;
            if (k == "percent_off") {
                discount_total += subtotal * (v / 100.0);
            } else if (k == "flat_off") {
                discount_total += v;
            } else {
                // unknown ignored
            }
        }

        // tax inline
        double tax = (subtotal - discount_total) * 0.18;
        double grand = subtotal - discount_total + tax;

        // rendering inline (pretend PDF)
        ostringstream pdf;
        pdf << "INVOICE\n";
        for (auto& it : items) {
            pdf << it.sku << " x" << it.quantity << " @ " << it.unitPrice << "\n";
        }
        pdf << "Subtotal: " << subtotal << "\n"
            << "Discounts: " << discount_total << "\n"
            << "Tax: " << tax << "\n"
            << "Total: " << grand << "\n";

        // email I/O inline (tight coupling)
        if (!email.empty()) {
            cout << "[SMTP] Sending invoice to " << email << "...\n";
        }

        // logging inline
        cout << "[LOG] Invoice processed for " << email << " total=" << grand << "\n";

        return pdf.str();
    }

    // helper used by ad-hoc tests; also messy on purpose
    double computeTotal(const vector<LineItem>& items,
                        const map<string, double>& discounts) {
        string dummyEmail = "noreply@example.com";
        auto rendered = process(items, discounts, dummyEmail);
        auto pos = rendered.rfind("Total:");
        if (pos == string::npos) throw runtime_error("No total");
        auto line = rendered.substr(pos + 6);
        return stod(line);
    }
};

int main() {
    InvoiceService svc; 
    // Create items
    vector<LineItem> items = { {"ITEM-001", 3, 100.0}, {"ITEM-002", 1, 250.0} };
    map<string,double> discounts = { {"percent_off", 10.0} };
    cout << svc.process(items, discounts, "customer@example.com") << endl;
    return 0;
}
