// // 01-invoice-ans.cpp
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <memory>

using namespace std;

struct LineItem {
    string sku;
    int quantity{0};
    double unitPrice{0.0};
};

// OCP: Discount Strategy
class IDiscount {
public:
    virtual ~IDiscount() = default;
    virtual double apply(double subtotal) const = 0;
};

class PercentDiscount : public IDiscount {
private:
    double percentage;
public:
    PercentDiscount(double p) : percentage(p) {}
    double apply(double subtotal) const override {
        return subtotal * (percentage / 100.0);
    }
};

class FlatDiscount : public IDiscount {
private:
    double amount;
public:
    FlatDiscount(double a) : amount(a) {}
    double apply(double subtotal) const override {
        return amount;
    }
};

class Invoice{
private:
    const vector<LineItem> items;
    const vector<shared_ptr<IDiscount>> discounts;
    const string email;

public:
    // Constructor for an immutable Invoice object
    Invoice(vector<LineItem> i, vector<shared_ptr<IDiscount>> d, string e)
        : items(move(i)), discounts(move(d)), email(move(e)) {}

    // Getters
    const vector<LineItem>& getItems() const { return items; }
    const vector<shared_ptr<IDiscount>>& getDiscounts() const { return discounts; }
    const string& getEmail() const { return email; }
};

// SRP / DIP: Abstractions
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(const string& message) = 0;
};

class IEmailSender {
public:
    virtual ~IEmailSender() = default;
    virtual void send(const string& to, const string& content) = 0;
};

class ITaxCalculator {
public:
    virtual ~ITaxCalculator() = default;
    virtual double calculate(double taxableAmount) = 0;
};

/**
 * @brief Interface for rendering an invoice.
 * 
 * This class defines the abstraction for rendering an invoice into a specific
 * format (e.g., text, HTML, PDF). It follows the Single Responsibility Principle
 * by separating rendering logic from the main invoice processing service.
 */
class IInvoiceRenderer {
public:
    virtual ~IInvoiceRenderer() = default;
    /**
     * @brief Renders the invoice data into a string format.
     * 
     * @param invoice The invoice data object.
     * @param subtotal The calculated subtotal before discounts and taxes.
     * @param discount The total discount amount applied.
     * @param tax The total tax amount applied.
     * @param grandTotal The final total amount.
     * @return A string representation of the invoice.
     */
    virtual string render(const Invoice& invoice, double subtotal, double discount, double tax, double grandTotal) = 0;
};

// SRP / DIP: Concrete Implementations
class ConsoleLogger : public ILogger {
public:
    void log(const string& message) override {
        cout << "[LOG] " << message << "\n";
    }
};

class SmtpEmailSender : public IEmailSender {
public:
    void send(const string& to, const string& content) override {
        cout << "[SMTP] Sending invoice to " << to << "...\n";
        // Pretend to send content
        (void)content;
    }
};

class FixedRateTaxCalculator : public ITaxCalculator {
private:
    double rate;
public:
    FixedRateTaxCalculator(double r) : rate(r) {}
    double calculate(double taxableAmount) override {
        return taxableAmount * rate;
    }
};

class InvoiceService {
private:
    shared_ptr<IInvoiceRenderer> renderer;
    shared_ptr<IEmailSender> emailer;
    shared_ptr<ILogger> logger;
    shared_ptr<ITaxCalculator> tax_calculator;

public:
    // DIP: Depend on abstractions, inject dependencies
    InvoiceService(shared_ptr<IInvoiceRenderer> renderer, shared_ptr<IEmailSender> emailer, shared_ptr<ILogger> logger, shared_ptr<ITaxCalculator> tax_calc)
        : renderer(renderer), emailer(emailer), logger(logger), tax_calculator(tax_calc) {}

    string process(Invoice &invoice) {
        const vector<LineItem>& items = invoice.getItems();
        const vector<shared_ptr<IDiscount>>& discounts = invoice.getDiscounts();
        const string& email = invoice.getEmail();
        // pricing
        double subtotal = 0.0;
        for (auto& it : items) subtotal += it.unitPrice * it.quantity;

        // OCP: Apply discount strategies
        double discount_total = 0.0;
        for (const auto& discount_strategy : discounts) {
            if (discount_strategy) {
                discount_total += discount_strategy->apply(subtotal);
            }
        }

        // SRP: Delegate tax calculation
        double taxable_amount = subtotal - discount_total;
        double tax = tax_calculator->calculate(taxable_amount);
        double grand = subtotal - discount_total + tax;

        // SRP: Delegate rendering
        string rendered_invoice = renderer->render(invoice, subtotal, discount_total, tax, grand);

        // SRP: Delegate emailing
        if (!email.empty()) {
            emailer->send(email, rendered_invoice);
        }

        // SRP: Delegate logging
        logger->log("Invoice processed for " + email + " total=" + to_string(grand));

        return rendered_invoice;
    }
};

// SRP: Concrete renderer implementation
class TextInvoiceRenderer : public IInvoiceRenderer {
public:
    string render(const Invoice& invoice, double subtotal, double discount, double tax, double grandTotal) override {
        ostringstream pdf;
        pdf << "INVOICE\n";
        for (const auto& it : invoice.getItems()) {
            pdf << it.sku << " x" << it.quantity << " @ " << it.unitPrice << "\n";
        }
        pdf << "Subtotal: " << subtotal << "\n"
            << "Discounts: " << discount << "\n"
            << "Tax: " << tax << "\n"
            << "Total: " << grandTotal << "\n";
        return pdf.str();
    }
};

// LSP Fix: Use composition, not inheritance
class InvoiceComputer {
private:
    InvoiceService& service;
public:
    // Helper used by ad-hoc tests; also messy on purpose
    explicit InvoiceComputer(InvoiceService& svc) : service(svc) {}

    double computeTotal(Invoice &invoice) {
        // Create a new invoice with a dummy email; avoids mutating the original.
        Invoice test_invoice(invoice.getItems(), invoice.getDiscounts(), "noreply@example.com");
        auto rendered = service.process(test_invoice);

        auto pos = rendered.rfind("Total:");
        if (pos == string::npos) throw runtime_error("No total");
        string line = rendered.substr(pos + 6);
        return stod(line);
    }
};

int main() {
    // DIP: Create concrete dependencies
    auto renderer = make_shared<TextInvoiceRenderer>();
    auto emailer = make_shared<SmtpEmailSender>();
    auto logger = make_shared<ConsoleLogger>();
    auto tax_calc = make_shared<FixedRateTaxCalculator>(0.18);

    // DIP: Inject dependencies into the high-level service
    InvoiceService svc(renderer, emailer, logger, tax_calc);

    vector<LineItem> items = { {"ITEM-001", 3, 100.0}, {"ITEM-002", 1, 250.0} };
    
    // Create discount strategy objects directly
    vector<shared_ptr<IDiscount>> discounts;
    discounts.push_back(make_shared<PercentDiscount>(10.0));

    string email = "customer@example.com";

    Invoice invoice(move(items), move(discounts), move(email));
    cout << svc.process(invoice) << endl;
    return 0;
}
