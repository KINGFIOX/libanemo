#include <cstddef>
#include <cstdint>
#include <cstring>
#include <libcpu/abstract_cpu.hh>
#include <libsdb/expression.hh>
#include <optional>
#include <regex>
#include <vector>

namespace libsdb {

struct tokenizer_rule_t {
  std::regex regex;
  token_type_t type;
  uint8_t precedence;
  uint8_t base;
};

static constexpr uint8_t max_prec = 8;
static const tokenizer_rule_t rules[] = {
    {std::regex("\\s+"), token_type_t::space, 0, 0},
    {std::regex("0b[01]+"), token_type_t::val, 0, 2},
    {std::regex("0o[0-7]+"), token_type_t::val, 0, 8},
    {std::regex("0x[0-9a-fA-F]+"), token_type_t::val, 0, 16},
    {std::regex("[0-9]+"), token_type_t::val, 0, 10},
    {std::regex("byte|half|word|sbyte|shalf|sword|pmem|vmem"), token_type_t::op,
     8, 0},
    {std::regex("<<|>>|>>>"), token_type_t::op, 5, 0},
    {std::regex(">=|<=|>|<|==|!="), token_type_t::op, 4, 0},
    {std::regex("[*/%]"), token_type_t::op, 7, 0},
    {std::regex("[+-]"), token_type_t::op, 6, 0},
    {std::regex("&"), token_type_t::op, 3, 0},
    {std::regex("\\^"), token_type_t::op, 2, 0},
    {std::regex("\\|"), token_type_t::op, 1, 0},
    {std::regex("[~!]"), token_type_t::op, 8, 0},
    {std::regex("\\("), token_type_t::parl, 0, 0},
    {std::regex("\\)"), token_type_t::parr, 0, 0},
    {std::regex("[a-z]+[0-9]*"), token_type_t::var, 0, 0}};

std::vector<token_t> tokenize_expression(const std::string &expr) {
  std::vector<token_t> tokens;

  std::string s = expr;
  std::smatch m;

  while (!s.empty()) {
    bool matched = false;
    for (const auto &rule : rules) {
      if (std::regex_search(s, m, rule.regex,
                            std::regex_constants::match_continuous)) {
        std::string token_str = m.str(0);
        matched = true;

        if (rule.type == token_type_t::space) {
          s = s.substr(token_str.length());
          break;
        }

        token_t token;
        switch (rule.type) {
        case token_type_t::val: {
          token.type = token_type_t::val;
          std::string num_str = token_str;
          if (rule.base != 10) {
            num_str = num_str.substr(2);
          }
          token.val = std::stoull(num_str, nullptr, rule.base);
          tokens.push_back(token);
          break;
        }
        case token_type_t::op: {
          token.type = token_type_t::op;
          std::strncpy(token.op.str, token_str.c_str(), 6);
          token.name[6] = 0;
          token.op.prec = rule.precedence;
          tokens.push_back(token);
          break;
        }
        case token_type_t::var: {
          token.type = token_type_t::var;
          std::strncpy(token.name, token_str.c_str(), 7);
          token.name[7] = 0;
          tokens.push_back(token);
          break;
        }
        case token_type_t::parl:
        case token_type_t::parr:
          token.type = rule.type;
          tokens.push_back(token);
          break;
        default:
          break;
        }
        s = s.substr(token_str.length());
        break;
      }
    }
    if (!matched) {
      tokens.push_back(token_t{token_type_t::invalid});
      break;
    }
  }
  return tokens;
}

std::optional<std::vector<token_t>>
parse_expression(const std::vector<token_t> &expr) {
  auto is_numerical = [](token_t token) {
    return token.type == token_type_t::val || token.type == token_type_t::var;
  };

  auto lilely_unary = [](token_t token) {
    if (token.type == token_type_t::op) {
      if (token.op.prec == max_prec) {
        return true;
      } else {
        char symbol = token.op.str[0];
        return symbol == '+' || symbol == '-';
      }
    } else {
      return false;
    }
  };

  // trivial cases
  if (expr.size() == 0) {
    return std::nullopt;
  }
  if (expr.size() == 1) {
    if (is_numerical(expr[0])) {
      return expr;
    } else {
      return std::nullopt;
    }
  }

  // find the last binary operator with lowest precedence
  for (uint8_t prec = 1; prec < max_prec; ++prec) {
    int par_cnt = 0;
    size_t op_index = 0;
    for (size_t i = 0; i < expr.size(); ++i) {
      if (expr[i].type == token_type_t::parl) {
        par_cnt += 1;
      } else if (expr[i].type == token_type_t::parr) {
        par_cnt -= 1;
      }

      if (par_cnt < 0) {
        return std::nullopt;
      } else if (par_cnt == 0 && // not inside any pair of parenthesis
                 expr[i].type == token_type_t::op &&
                 expr[i].op.prec ==
                     prec && // is an operator of specified precedence
                 i > 0 &&
                 (is_numerical(expr[i - 1]) ||
                  expr[i - 1].type ==
                      token_type_t::parr) // is a binary operator
      ) {
        op_index = i;
      }
    }

    if (par_cnt != 0) {
      return std::nullopt;
    } else if (op_index != 0) {
      auto op1 = parse_expression({expr.begin(), expr.begin() + op_index});
      auto op2 = parse_expression({expr.begin() + op_index + 1, expr.end()});
      if (op1.has_value() && op2.has_value()) {
        std::vector<token_t> result{};
        result.insert(result.end(), op1->begin(), op1->end());
        result.insert(result.end(), op2->begin(), op2->end());
        result.push_back(expr[op_index]);
        return result;
      } else {
        return std::nullopt;
      }
    }
  }

  // now there cannot be binary operators outside parenthesis
  // remove outter parenthesis
  if (expr.front().type == token_type_t::parl && expr.size() >= 3 &&
      expr.back().type == token_type_t::parr) {
    return parse_expression({expr.begin() + 1, expr.end() - 1});
  }

  // now it can only begin with a unary operator
  if (lilely_unary(expr[0])) {
    auto op1 = parse_expression({expr.begin() + 1, expr.end()});
    if (op1.has_value()) {
      op1->push_back(expr[0]);
      // this makes unary and binary `+`, `-` different
      // `max_prec` indicates that it is unary
      op1->back().op.prec = max_prec;
      return op1;
    } else {
      return std::nullopt;
    }
  } else {
    return std::nullopt;
  }
}

} // namespace libsdb
