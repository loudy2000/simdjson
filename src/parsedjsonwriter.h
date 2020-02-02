#ifndef __PARSED_JSON_WRITER_H__
#define __PARSED_JSON_WRITER_H__

namespace simdjson {

class ParsedJsonWriter {
public:
  ParsedJson &pj;

  really_inline ParsedJsonWriter(ParsedJson &_pj) : pj{_pj}, current_loc{0}, current_string_buf_loc{pj.string_buf.get()} {
    pj.valid = false;
  }

  really_inline ErrorValues on_error(ErrorValues error_code) {
    pj.error_code = error_code;
    return error_code;
  }
  really_inline ErrorValues on_success(ErrorValues success_code) {
    pj.error_code = success_code;
    pj.valid = true;
    return success_code;
  }
  really_inline bool on_start_document(uint32_t depth) {
    pj.containing_scope_offset[depth] = get_current_loc();
    write_tape(0, 'r');
    return true;
  }
  really_inline bool on_start_object(uint32_t depth) {
    pj.containing_scope_offset[depth] = get_current_loc();
    write_tape(0, '{');
    return true;
  }
  really_inline bool on_start_array(uint32_t depth) {
    pj.containing_scope_offset[depth] = get_current_loc();
    write_tape(0, '[');
    return true;
  }
  // TODO we're not checking this bool
  really_inline bool on_end_document(uint32_t depth) {
    // write our tape location to the header scope
    // The root scope gets written *at* the previous location.
    annotate_previous_loc(pj.containing_scope_offset[depth], get_current_loc());
    write_tape(pj.containing_scope_offset[depth], 'r');
    return true;
  }
  really_inline bool on_end_object(uint32_t depth) {
    // write our tape location to the header scope
    write_tape(pj.containing_scope_offset[depth], '}');
    annotate_previous_loc(pj.containing_scope_offset[depth], get_current_loc());
    return true;
  }
  really_inline bool on_end_array(uint32_t depth) {
    // write our tape location to the header scope
    write_tape(pj.containing_scope_offset[depth], ']');
    annotate_previous_loc(pj.containing_scope_offset[depth], get_current_loc());
    return true;
  }

  really_inline bool on_true_atom() {
    write_tape(0, 't');
    return true;
  }
  really_inline bool on_false_atom() {
    write_tape(0, 'f');
    return true;
  }
  really_inline bool on_null_atom() {
    write_tape(0, 'n');
    return true;
  }

  really_inline uint8_t *on_start_string() {
    /* we advance the point, accounting for the fact that we have a NULL
      * termination         */
    write_tape(current_string_buf_loc - pj.string_buf.get(), '"');
    return current_string_buf_loc + sizeof(uint32_t);
  }

  really_inline bool on_end_string(uint8_t *dst) {
    uint32_t str_length = dst - (current_string_buf_loc + sizeof(uint32_t));
    // TODO check for overflow in case someone has a crazy string (>=4GB?)
    // But only add the overflow check when the document itself exceeds 4GB
    // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
    memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
    // NULL termination is still handy if you expect all your strings to
    // be NULL terminated? It comes at a small cost
    *dst = 0;
    current_string_buf_loc = dst + 1;
    return true;
  }

  really_inline bool on_number_s64(int64_t value) {
    write_tape(0, 'l');
    std::memcpy(&pj.tape[current_loc], &value, sizeof(value));
    ++current_loc;
    return true;
  }
  really_inline bool on_number_u64(uint64_t value) {
    write_tape(0, 'u');
    pj.tape[current_loc++] = value;
    return true;
  }
  really_inline bool on_number_double(double value) {
    write_tape(0, 'd');
    static_assert(sizeof(double) == sizeof(pj.tape[current_loc]), "mismatch size");
    memcpy(&pj.tape[current_loc++], &value, sizeof(double));
    // pj.tape[current_loc++] = *((uint64_t *)&value);
    return true;
  }

private:
  uint32_t current_loc{0};
  uint8_t *current_string_buf_loc;

  // this should be considered a private function
  really_inline void write_tape(uint64_t val, uint8_t c) {
    pj.tape[current_loc++] = val | ((static_cast<uint64_t>(c)) << 56);
  }

  really_inline uint32_t get_current_loc() const { return current_loc; }

  really_inline void annotate_previous_loc(uint32_t saved_loc, uint64_t val) {
    pj.tape[saved_loc] |= val;
  }
};

} // namespace simdjson

#endif // __PARSED_JSON_WRITER_H__
