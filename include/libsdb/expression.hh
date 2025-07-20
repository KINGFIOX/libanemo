#ifndef LIBSDB_EXPRESSION_HH
#define LIBSDB_EXPRESSION_HH

#include <cstdint>
#include <cstring>
#include <libcpu/abstract_cpu.hh>
#include <type_traits>
#include <vector>
#include <string>
#include <optional>
#include <libvio/width.hh>

namespace libsdb {

/**
 * @file 
 * @brief Token definitions for a lexer/parser system.
 */

/**
 * @enum token_type_t
 * @brief Enumeration of possible token types.
 */
enum class token_type_t {
    space,  ///< White space
    val,    ///< Constant numeric value
    pc,     ///< Program counter of the guest
    reg,    ///< Register of the guest
    op,     ///< Operator
    var,    ///< Variable
    parl,   ///< Left parenthesis '('
    parr,   ///< Right parenthesis ')'
    invalid ///< Invalid/unknown token
};

/**
 * @struct token_t
 * @brief Token structure containing type and value information.
 * 
 * Uses a union to store different types of token data efficiently.
 */
struct token_t {
    token_type_t type; ///< Type of the token
    
    /**
     * @union 
     * @brief Union containing token value data
     * 
     * The active member depends on the token type:
     * - val/reg: use `val`
     * - op: use `op`
     * - var: use `name`
     * - pc/parl/parr/invalid: no value needed
     */
    union {
        uint64_t val;   ///< Numeric value (for val tokens or register address)
        char name[8];   ///< Variable name (for var tokens)
        /**
         * @struct op
         * @brief Operator information
         * All binary operators are evaluated from left to right
         * All unary operators are evaluated from right to left
         * All unary operators have the same precedence (higher than binary operators).
         */
        struct {
            char str[7];    ///< Operator string representation
            uint8_t prec;   ///< Operator precedence, only need for binary operators
        } op;
    };
};

/**
 * @brief Tokenizes an input expression string
 * @param expr The input expression string to tokenize
 * @return Vector of tokens representing the tokenized expression
 */
std::vector<token_t> tokenize_expression(const std::string &expr);

/**
 * @brief Parses a tokenized expression into postfix notation (RPN)
 * @param expr Vector of tokens representing the tokenized expression
 * @return Vector of tokens in postfix notation if parsing succeeds, nullopt if syntax error
 */
std::optional<std::vector<token_t>> parse_expression(const std::vector<token_t> &expr);

/**
 * @brief Specializes an expression by resolving variables to specific CPU registers
 *
 * Expressions can be evaluated without specializing.
 * However, if an expression will be evalueated multiple times,
 * Specializing helps improve performace.
 *
 * @tparam WORD_T The word type of the CPU (template parameter)
 * @param expr The expression to specialize (modified in-place)
 * @param cpu Reference to the abstract CPU implementation
 */
template <typename WORD_T>
void specialize_expression(std::vector<token_t> &expr, const libcpu::abstract_cpu<WORD_T> &cpu) {
    for (auto &token: expr) {
        if (token.type == token_type_t::var) {
            if (strcmp(token.name, "pc") == 0) {
                token.type = token_type_t::pc;
            } else {
                token_t new_token;
                new_token.type = token_type_t::reg;
                new_token.val = cpu.gpr_addr(token.name);
                token = new_token;
            }
        }
    }
}

/**
 * @brief Evaluates a postfix expression
 *
 * `cpu` can be nullptr in case you are evaluating an expression not related to any simulated CPU.
 *
 * @tparam WORD_T The word type, must be unsigned integer. All numbers are converted to this type.
 * @param postfix_expr The expression in postfix notation (RPN)
 * @param cpu Pointer to the CPU implementation. If nullptr, the expression cannot refer to CPU registers.
 * @return Optional result of the evaluation if successful
 */
template <typename WORD_T>
std::optional<WORD_T> evaluate_expression(const std::vector<token_t> &postfix_expr, const libcpu::abstract_cpu<WORD_T> *cpu) {
    using SWORD_T = std::make_signed<WORD_T>;
    std::vector<WORD_T> stack;
    
    for (const auto &token : postfix_expr) {
        switch (token.type) {
            case token_type_t::val:
                stack.push_back(token.val);
                break;
                
            case token_type_t::var:
                // Extract variable name (max 8 chars)
                if (cpu != nullptr) {
                    if (strncmp(token.name, "pc", 2) == 0) {
                        stack.push_back(cpu->get_pc());
                    } else {
                        stack.push_back(cpu->get_gpr(cpu->gpr_addr(token.name)));
                    }
                    
                } else {
                    return std::nullopt;
                }
                break;

            case token_type_t::reg:
                if (cpu != nullptr) {
                    stack.push_back(cpu->get_gpr(token.val));
                } else {
                    return std::nullopt;
                }
                break;

            case token_type_t::pc:
                if (cpu != nullptr) {
                    stack.push_back(cpu->get_pc());
                } else {
                    return std::nullopt;
                }
                break;
                
            case token_type_t::op:
                if (token.op.prec == 8) {  // Unary operator
                    if (stack.empty()) {
                        return {};
                    }

                    uint64_t operand = stack.back();
                    stack.pop_back();
                    
                    if (strcmp(token.op.str, "~") == 0) {
                        stack.push_back(~operand);
                    } else if (strcmp(token.op.str, "!") == 0) {
                        stack.push_back(operand ? 0 : 1);
                    } else if (strcmp(token.op.str, "+") == 0) {
                        stack.push_back(operand);
                    } else if (strcmp(token.op.str, "-") == 0) {
                        stack.push_back(0 - operand);
                    } else if (strcmp(token.op.str, "byte") == 0) {
                        stack.push_back(libvio::zero_truncate(operand, libvio::width_t::byte));
                    } else if (strcmp(token.op.str, "half") == 0) {
                        stack.push_back(libvio::zero_truncate(operand, libvio::width_t::half));
                    } else if (strcmp(token.op.str, "word") == 0) {
                        stack.push_back(libvio::zero_truncate(operand, libvio::width_t::word));
                    } else if (strcmp(token.op.str, "sbyte") == 0) {
                        stack.push_back(libvio::sign_extend(operand, libvio::width_t::byte));
                    } else if (strcmp(token.op.str, "shalf") == 0) {
                        stack.push_back(libvio::sign_extend(operand, libvio::width_t::half));
                    } else if (strcmp(token.op.str, "sword") == 0) {
                        stack.push_back(libvio::sign_extend(operand, libvio::width_t::word));
                    } else if (strcmp(token.op.str, "pmem") == 0) {
                        if (cpu != nullptr) {
                            std::optional<WORD_T> val = cpu->pmem_peek(operand, static_cast<libvio::width_t>(sizeof(WORD_T)));
                            if (val.has_value()) {
                                stack.push_back(val.value());
                            } else {
                                return {};
                            }
                        } else {
                            return {};
                        }
                    } else if (strcmp(token.op.str, "vmem") == 0) {
                        if (cpu != nullptr) {
                            std::optional<WORD_T> val = cpu->vmem_peek(operand, static_cast<libvio::width_t>(sizeof(WORD_T)));
                            if (val.has_value()) {
                                stack.push_back(val.value());
                            } else {
                                return {};
                            }
                        } else {
                            return {};
                        }
                    } else {
                        return {};
                    }
                } else {  // Binary operator
                    if (stack.size() < 2) {
                        return {};
                    }
                    uint64_t right = stack.back();
                    stack.pop_back();
                    uint64_t left = stack.back();
                    stack.pop_back();
                    
                    if (strcmp(token.op.str, "<<") == 0) {
                        uint64_t shift = right & 0x3F;
                        stack.push_back(left << shift);
                    } else if (strcmp(token.op.str, ">>") == 0) {
                        uint64_t shift = right & 0x3F;
                        stack.push_back(static_cast<uint64_t>(static_cast<int64_t>(left) >> shift));
                    } else if (strcmp(token.op.str, ">>>") == 0) {
                        uint64_t shift = right & 0x3F;
                        stack.push_back(left >> shift);
                    } else if (strcmp(token.op.str, ">=") == 0) {
                        stack.push_back(left >= right ? 1 : 0);
                    } else if (strcmp(token.op.str, "<=") == 0) {
                        stack.push_back(left <= right ? 1 : 0);
                    } else if (strcmp(token.op.str, ">") == 0) {
                        stack.push_back(left > right ? 1 : 0);
                    } else if (strcmp(token.op.str, "<") == 0) {
                        stack.push_back(left < right ? 1 : 0);
                    } else if (strcmp(token.op.str, "==") == 0) {
                        stack.push_back(left == right ? 1 : 0);
                    } else if (strcmp(token.op.str, "!=") == 0) {
                        stack.push_back(left != right ? 1 : 0);
                    } else if (strcmp(token.op.str, "*") == 0) {
                        stack.push_back(left * right);
                    } else if (strcmp(token.op.str, "/") == 0) {
                        if (right == 0) {
                            return {};
                        }
                        stack.push_back(left / right);
                    } else if (strcmp(token.op.str, "%") == 0) {
                        if (right == 0) {
                            return {};
                        }
                        stack.push_back(left % right);
                    } else if (strcmp(token.op.str, "+") == 0) {
                        stack.push_back(left + right);
                    } else if (strcmp(token.op.str, "-") == 0) {
                        stack.push_back(left - right);
                    } else if (strcmp(token.op.str, "&") == 0) {
                        stack.push_back(left & right);
                    } else if (strcmp(token.op.str, "^") == 0) {
                        stack.push_back(left ^ right);
                    } else if (strcmp(token.op.str, "|") == 0) {
                        stack.push_back(left | right);
                    } else {
                        return {};
                    }
                }
                break;
                
            default:
                return std::nullopt;
        }
    }
    
    if (stack.size() != 1) {
        return {};
    } else {
        return stack.back();
    }
}

/**
 * @brief Evaluates an expression represented with a string
 *
 * `cpu` can be nullptr in case you are evaluating an expression not related to any simulated CPU.
 *
 * @tparam WORD_T The word type, must be unsigned integer. All numbers are converted to this type.
 * @param expr The expression
 * @param cpu Pointer to the CPU implementation. If nullptr, the expression cannot refer to CPU registers.
 * @return Optional result of the evaluation if successful
 */
template <typename WORD_T>
std::optional<WORD_T> evaluate_expression(const std::string &expr, const libcpu::abstract_cpu<WORD_T> *cpu) {
    return evaluate_expression(parse_expression(tokenize_expression(expr)).value_or(std::vector<token_t>{}), cpu);
}

}

#endif
