#pragma once

#ifdef USE_EXPRTK

#include "exprtk.hpp"
#include <QString>
#include <QHash>
#include <functional>
#include <memory>
#include <vector>
#include <utility>

// Expression engine wrapping ExprTk for runtime formula evaluation.
// Three-component architecture: SymbolTable + Expression + Parser.
// Supports user-defined boundary conditions, flow control, integration/differentiation.

namespace ExpressionEngine {

class Script {
public:
    Script();
    ~Script();

    // Register a scalar variable accessible in expressions
    // e.g., addVariable("density", &m_density)
    bool addVariable(const QString& name, double& ref);

    // Register a constant (immutable in expressions)
    bool addConstant(const QString& name, double value);

    // Register a string variable for flow control scripts
    bool addStringVariable(const QString& name, std::string& ref);

    using ScalarFunc = std::function<double(double)>;
    using ScalarFunc2 = std::function<double(double, double)>;

    // Register a user-defined unary function
    // e.g., addFunction("visc", [](double T){ return 1e-5*exp(0.02*T); })
    bool addFunction(const QString& name, ScalarFunc func);

    // Register a binary function
    bool addFunction2(const QString& name, ScalarFunc2 func);

    // Compile an expression string. Returns true on success.
    bool compile(const QString& expression);

    // Evaluate the compiled expression, returns the result
    double evaluate();

    // Get compile error message if compile() returned false
    QString errorString() const { return m_error; }

    // Check if compiled and ready to evaluate
    bool isValid() const { return m_compiled; }

    // Define a function f(x) from an expression with variable 'x'
    bool defineScalarFunction(const QString& funcExpression);

    // Compute derivative df/dx at point x0 using central difference
    double derivative(double x0, double delta = 1e-6);

    // Compute second derivative d²f/dx² at x0
    double secondDerivative(double x0, double delta = 1e-6);

    // Compute numerical integral ∫f(x)dx from a to b
    double integral(double a, double b, int numIntervals = 1000);

    // Set max loop iterations (safety limit, default 10000)
    void setMaxLoopIterations(int max) { m_maxLoops = max; }

    // Set max recursion depth
    void setMaxRecursionDepth(int max) { m_maxRecursion = max; }

private:
    exprtk::symbol_table<double> m_symbolTable;
    exprtk::expression<double> m_expression;
    exprtk::parser<double> m_parser;

    // Owned storage for functions (ExprTk needs them alive for expression lifetime)
    std::vector<std::unique_ptr<ScalarFunc>> m_functions;
    std::vector<std::unique_ptr<ScalarFunc2>> m_functions2;

    // Owned storage for variables referenced by symbol table
    std::vector<std::pair<QString, double>> m_variableStorage;
    std::vector<std::pair<QString, std::string>> m_stringStorage;

    QString m_error;
    bool m_compiled = false;
    int m_maxLoops = 10000;
    int m_maxRecursion = 100;

    // Tracked variable refs for derivative/integral
    double m_xVar = 0.0;
    bool m_hasFunctionDef = false;
};

// ─── Implementation ──────────────────────────────────────────────

inline Script::Script()
{
    m_symbolTable.add_constants();
    m_expression.register_symbol_table(m_symbolTable);
}

inline Script::~Script() = default;

inline bool Script::addVariable(const QString& name, double& ref)
{
    if (!m_symbolTable.add_variable(name.toStdString(), ref)) {
        m_error = QStringLiteral("Failed to add variable: %1").arg(name);
        return false;
    }
    return true;
}

inline bool Script::addConstant(const QString& name, double value)
{
    // Store value so it outlives the call
    m_variableStorage.emplace_back(name, value);
    if (!m_symbolTable.add_constant(name.toStdString(),
                                     m_variableStorage.back().second)) {
        m_error = QStringLiteral("Failed to add constant: %1").arg(name);
        return false;
    }
    return true;
}

inline bool Script::addStringVariable(const QString& name, std::string& ref)
{
    if (!m_symbolTable.add_stringvar(name.toStdString(), ref)) {
        m_error = QStringLiteral("Failed to add string variable: %1").arg(name);
        return false;
    }
    return true;
}

inline bool Script::addFunction(const QString& name, ScalarFunc func)
{
    auto ptr = std::make_unique<ScalarFunc>(std::move(func));
    auto raw = ptr.get();
    m_functions.push_back(std::move(ptr));

    if (!m_symbolTable.add_function(name.toStdString(),
          [raw](double x) { return (*raw)(x); })) {
        m_error = QStringLiteral("Failed to add function: %1").arg(name);
        return false;
    }
    return true;
}

inline bool Script::addFunction2(const QString& name, ScalarFunc2 func)
{
    auto ptr = std::make_unique<ScalarFunc2>(std::move(func));
    auto raw = ptr.get();
    m_functions2.push_back(std::move(ptr));

    if (!m_symbolTable.add_function(name.toStdString(),
          [raw](double x, double y) { return (*raw)(x, y); })) {
        m_error = QStringLiteral("Failed to add function: %1").arg(name);
        return false;
    }
    return true;
}

inline bool Script::compile(const QString& expression)
{
    m_compiled = false;
    std::string exprStr = expression.toStdString();

    // Enable flow control, loops
    m_parser.settings().set_max_number_of_loops(m_maxLoops);

    if (!m_parser.compile(exprStr, m_expression)) {
        m_error = QString::fromStdString(m_parser.error());
        return false;
    }
    m_compiled = true;
    return true;
}

inline double Script::evaluate()
{
    if (!m_compiled) return 0.0;
    return m_expression.value();
}

inline bool Script::defineScalarFunction(const QString& funcExpression)
{
    m_variableStorage.emplace_back("x", 0.0);
    m_xVar = 0.0;
    m_variableStorage.back().second = m_xVar;

    if (!m_symbolTable.add_variable("x", m_variableStorage.back().second)) {
        m_error = QStringLiteral("Failed to add variable 'x'");
        return false;
    }

    if (!compile(funcExpression)) return false;
    m_hasFunctionDef = true;
    return true;
}

inline double Script::derivative(double x0, double delta)
{
    if (!m_compiled || !m_hasFunctionDef) return 0.0;
    m_variableStorage.back().second = x0 + delta;
    double fp = evaluate();
    m_variableStorage.back().second = x0 - delta;
    double fm = evaluate();
    m_variableStorage.back().second = x0;
    return (fp - fm) / (2.0 * delta);
}

inline double Script::secondDerivative(double x0, double delta)
{
    if (!m_compiled || !m_hasFunctionDef) return 0.0;
    m_variableStorage.back().second = x0 + delta;
    double fp = evaluate();
    m_variableStorage.back().second = x0;
    double f0 = evaluate();
    m_variableStorage.back().second = x0 - delta;
    double fm = evaluate();
    m_variableStorage.back().second = x0;
    return (fp - 2.0 * f0 + fm) / (delta * delta);
}

inline double Script::integral(double a, double b, int numIntervals)
{
    if (!m_compiled || !m_hasFunctionDef || numIntervals <= 0) return 0.0;
    double dx = (b - a) / numIntervals;
    double sum = 0.0;
    for (int i = 0; i < numIntervals; ++i) {
        double x1 = a + i * dx;
        double x2 = x1 + dx;
        m_variableStorage.back().second = x1;
        double f1 = evaluate();
        m_variableStorage.back().second = x2;
        double f2 = evaluate();
        sum += 0.5 * (f1 + f2) * dx; // trapezoidal rule
    }
    return sum;
}

// Factory: create a script pre-loaded with standard fluid dynamics variables
// Variables: P(pressure), T(temperature), rho(density), v(velocity),
//            mdot(massFlow), d(diameter), L(length), mu(viscosity),
//            Re(Reynolds), zeta(lossCoeff)
inline std::unique_ptr<Script> createFluidDynamicsScript()
{
    auto s = std::make_unique<Script>();
    s->addVariable("P",    *new double(0.0)); // caller must manage lifetime
    s->addVariable("T",    *new double(0.0));
    s->addVariable("rho",  *new double(0.0));
    s->addVariable("v",    *new double(0.0));
    s->addVariable("mdot", *new double(0.0));
    s->addVariable("d",    *new double(0.0));
    s->addVariable("L",    *new double(0.0));
    s->addVariable("mu",   *new double(0.0));
    s->addVariable("Re",   *new double(0.0));
    s->addVariable("zeta", *new double(0.0));
    s->addConstant("pi", M_PI);
    s->addConstant("g0", 9.80665);
    return s;
}

} // namespace ExpressionEngine

#else // !USE_EXPRTK — stub implementation

#include <QString>
#include <functional>
#include <memory>

namespace ExpressionEngine {

class Script {
public:
    bool addVariable(const QString&, double&) { return false; }
    bool addConstant(const QString&, double) { return false; }
    bool addStringVariable(const QString&, std::string&) { return false; }

    using ScalarFunc = std::function<double(double)>;
    using ScalarFunc2 = std::function<double(double, double)>;
    bool addFunction(const QString&, ScalarFunc) { return false; }
    bool addFunction2(const QString&, ScalarFunc2) { return false; }

    bool compile(const QString&) { return false; }
    double evaluate() { return 0.0; }
    QString errorString() const { return QStringLiteral("ExprTk not available"); }
    bool isValid() const { return false; }

    bool defineScalarFunction(const QString&) { return false; }
    double derivative(double, double = 1e-6) { return 0.0; }
    double secondDerivative(double, double = 1e-6) { return 0.0; }
    double integral(double, double, int = 1000) { return 0.0; }

    void setMaxLoopIterations(int) {}
    void setMaxRecursionDepth(int) {}
};

inline std::unique_ptr<Script> createFluidDynamicsScript() { return nullptr; }

} // namespace ExpressionEngine

#endif // USE_EXPRTK
