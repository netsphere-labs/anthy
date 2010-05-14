%module anthy
%include typemaps.i

%{
#include <include/anthy.h>
%}

%init %{
anthy_init();
%}

%define OUTPUT_TYPEMAP(type, klass)
%typemap(in,numinputs=0) type *OUTPUT(VALUE temp)
{
  temp = rb_funcall(klass, rb_intern("new"), 0);
  Data_Get_Struct(temp, $*ltype, $1);
}

%typemap(argout) type *OUTPUT
{
  $result = temp$argnum;
}
%enddef

%rename(ConvStat) anthy_conv_stat;
%rename(SegmentStat) anthy_segment_stat;

OUTPUT_TYPEMAP(struct anthy_conv_stat, cConvStat.klass);
OUTPUT_TYPEMAP(struct anthy_segment_stat, cSegmentStat.klass);

#undef OUTPUT_TYPEMAP

%rename(COMPILED_ENCODING) ANTHY_COMPILED_ENCODING;
%rename(EUC_JP_ENCODING) ANTHY_EUC_JP_ENCODING;
%rename(UTF_8_ENCODING) ANTHY_UTF_8_ENCODING;

%rename(init) anthy_init;
%rename(quit) anthy_quit;
%rename(conf_override) anthy_conf_override;
%rename(set_personality) anthy_set_personality;

%ignore anthy_create_context;
%ignore anthy_reset_context;
%ignore anthy_release_context;

%ignore anthy_set_string;
%ignore anthy_resize_segment;
%ignore anthy_get_stat;
%ignore anthy_get_segment_stat;
%ignore anthy_get_segment;
%ignore anthy_commit_segment;
%ignore anthy_print_context;
%ignore anthy_context_set_encoding;

%{
#define anthy_context_set_string anthy_set_string
#define anthy_context_resize_segment anthy_resize_segment
#define anthy_context_get_stat anthy_get_stat
#define anthy_context_get_segment_stat anthy_get_segment_stat
/* #define anthy_context_get_segment anthy_get_segment */
#define anthy_context_commit_segment anthy_commit_segment
#define anthy_context_print anthy_print_context
%}

%rename(Context) anthy_context;

struct anthy_context
{
  %extend 
  {
    anthy_context(void)
      {
        return anthy_create_context();
      }
    ~anthy_context(void)
      {
        anthy_release_context(self);
      }

    void reset(void)
      {
        anthy_reset_context(self);
      };

    int set_string(char *);
    void resize_segment(int, int);
    int get_stat(struct anthy_conv_stat *OUTPUT);
    int get_segment_stat(int, struct anthy_segment_stat *OUTPUT);

    VALUE get_segment(int nth_seg, int nth_cand)
      {
        int len;
        char *buffer;
        len = anthy_get_segment(self, nth_seg, nth_cand, NULL, 0);
        buffer = alloca(len + 1);
        len = anthy_get_segment(self, nth_seg, nth_cand, buffer, len + 1);
        if (len > 0) {
          return rb_str_new2(buffer);
        } else {
          return Qnil;
        }
      }

    int commit_segment(int, int);

    void print();

    void set_encoding(int encoding);

  }
};

%rename(get_version_string) anthy_get_version_string;
/* typedef void (*anthy_logger)(int level, const char *); */
%rename(set_logger) anthy_set_logger;
%ignore anthy_set_logger;

%include ../include/anthy.h
