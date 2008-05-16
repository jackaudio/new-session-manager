# -*- mode: makefile; -*-

FL_SRCS= \
   FL/Fl_Scalepack.C \
   FL/Fl_Arc_Dial.C \
   FL/Fl_Text_Edit_Window.C \
   FL/Boxtypes.C \

FL_OBJS := $(FL_SRCS:.C=.o)

all: FL/libfl_widgets.a

# $(OBJS): Makefile

FL/libfl_widgets.a: $(FL_OBJS)
	@ ar rcs $@ $(FL_OBJS)

.PHONEY: FL
FL: FL/libfl_widgets.a

FL_clean:
	rm -f $(FL_OBJS) FL/libfl_widgets.a
