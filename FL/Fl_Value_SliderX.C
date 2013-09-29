
/*******************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                                     */
/*                                                                             */
/* This program is free software; you can redistribute it and/or modify it     */
/* under the terms of the GNU General Public License as published by the       */
/* Free Software Foundation; either version 2 of the License, or (at your      */
/* option) any later version.                                                  */
/*                                                                             */
/* This program is distributed in the hope that it will be useful, but WITHOUT */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   */
/* more details.                                                               */
/*                                                                             */
/* You should have received a copy of the GNU General Public License along     */
/* with This program; see the file COPYING.  If not,write to the Free Software */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
/*******************************************************************************/


#include "Fl_Value_SliderX.H"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <math.h>

void Fl_Value_SliderX::input_cb(Fl_Widget*, void* v) {
    Fl_Value_SliderX& t = *(Fl_Value_SliderX*)v;
    double nv;
    if ((t.step() - floor(t.step()))>0.0 || t.step() == 0.0)
        nv = strtod(t.input.value(), 0);
    else
        nv = strtol(t.input.value(), 0, 0);

    if (nv != t.value() || t.when() & FL_WHEN_NOT_CHANGED) {

        if ( ! t.soft()) 
            nv = t.clamp(nv);
        t.set_value(nv);
        t.set_changed();
        if (t.when())
        {
            t.value_damage();
            t.do_callback();
        }
    }
}

void Fl_Value_SliderX::value_damage() {
    char buf[128];
    format(buf);
    input.value(buf);
    input.mark(input.position()); // turn off selection highlight
    redraw();
}


Fl_Value_SliderX::~Fl_Value_SliderX ( void )
{
    if (input.parent() == (Fl_Group *)this)
        input.parent(0);   // *revert* ctor kludge!
}

/**
   Creates a new Fl_Value_SliderX widget using the given
   position, size, and label string. The default boxtype is FL_DOWN_BOX.
*/
Fl_Value_SliderX::Fl_Value_SliderX(int X, int Y, int W, int H, const char*l)
    : Fl_SliderX(X,Y,W,H,l),input(X, Y, W, H, 0) {
    step(1,100);

    soft_ = 0;
    if (input.parent())  // defeat automatic-add
        input.parent()->remove(input);
    input.parent((Fl_Group *)this); // kludge!
    input.callback(input_cb, this);
    input.when(FL_WHEN_ENTER_KEY);
    align(FL_ALIGN_LEFT);
    value_damage();
    textsize(9);
    set_flag(SHORTCUT_LABEL);

}

void Fl_Value_SliderX::draw() {

    int sxx = x(), syy = y(), sww = w(), shh = h();
    int bxx = x(), byy = y(), bww = w(), bhh = h();
    if (horizontal()) {
        input.resize(x(), y(), 35, h());
        bww = 35; sxx += 35; sww -= 35;
    } else {
        input.resize(x(), y(), w(), 25 );
        syy += 25; bhh = 25; shh -= 25;
    }
    if (damage()&FL_DAMAGE_ALL) draw_box(box(),sxx,syy,sww,shh,color());
    Fl_SliderX::draw(sxx+Fl::box_dx(box()),
                     syy+Fl::box_dy(box()),
                     sww-Fl::box_dw(box()),
                     shh-Fl::box_dh(box()));
    draw_box(box(),bxx,byy,bww,bhh,color());

    if (damage()&~FL_DAMAGE_CHILD) input.clear_damage(FL_DAMAGE_ALL);
    input.box(box());
    input.color(color(), selection_color());
    Fl_Widget *i = &input; i->draw(); // calls protected input.draw()
    input.clear_damage();
}

int Fl_Value_SliderX::handle(int event) {
    if (event == FL_PUSH && Fl::visible_focus()) {
        Fl::focus(this);
        redraw();
    }

    int sxx = x(), syy = y(), sww = w(), shh = h();
    if (horizontal()) {
        sxx += 35; sww -= 35;
    } else {
        syy += 25; shh -= 25;
    }

    double v;
    int delta;
    int mx = Fl::event_x_root();
    static int ix, drag;
//    input.when(when());
    switch (event) {
        case FL_ENTER:
            return 1;
        case FL_LEAVE:
            if ( ! drag )
                fl_cursor( FL_CURSOR_DEFAULT );
            return 1;
        case FL_MOVE:
            if ( drag || Fl::event_inside( &input ) )
                fl_cursor( FL_CURSOR_WE );
            else
                fl_cursor( FL_CURSOR_DEFAULT );
            return 1;
        case FL_PUSH:
//            if (!step()) goto DEFAULT;
            if ( Fl::event_inside(&input) )
            {
                input.handle(event);
                ix = mx;
                drag = Fl::event_button();
                handle_push();
                return 1;
            }
            goto DEFAULT;
            break;
        case FL_DRAG:
            if ( ! drag )
                goto DEFAULT;

            fl_cursor( FL_CURSOR_WE );
                
//            if (!step()) goto DEFAULT;
            delta = mx-ix;
            if (!horizontal())
                delta = -delta;

            if (delta > 5) delta -= 5;
            else if (delta < -5) delta += 5;
            else delta = 0;

            switch (drag) {
                case 3: v = increment(previous_value(), delta*100); break;
                case 2: v = increment(previous_value(), delta*10); break;
                default:v = increment(previous_value(), delta); break;
            }
            
//            v = previous_value() + delta * ( fabs( maximum() - minimum() ) * 0.001 );

            v = round(v);
            v = soft()?softclamp(v):clamp(v);
            handle_drag(v);
            value_damage();
            return 1;
        case FL_RELEASE:

            if ( ! drag )
                goto DEFAULT;

            //          if (!step()) goto DEFAULT;
            if (value() != previous_value() || !Fl::event_is_click())
                handle_release();

            drag = 0;

            fl_cursor( FL_CURSOR_DEFAULT );

            /* else { */
            /*   Fl_Widget_Tracker wp(&input); */
            /*   input.handle(FL_PUSH); */
            /*   if (wp.exists()) */
            /*     input.handle(FL_RELEASE); */
            /* } */
            return 1;
        case FL_FOCUS:
            return input.take_focus();
        case FL_UNFOCUS:
        {
            input_cb(&input,this);
            return 1;
        }
        case FL_PASTE:
            return 0;
        case FL_SHORTCUT:
            return input.handle(event);
    }

DEFAULT:
 
    int r = Fl_SliderX::handle(event,
                               sxx+Fl::box_dx(box()),
                               syy+Fl::box_dy(box()),
                               sww-Fl::box_dw(box()),
                               shh-Fl::box_dh(box()));

    if ( r )
    {
        return r;
    }
    else
    {
        input.type(((step() - floor(step()))>0.0 || step() == 0.0) ? FL_FLOAT_INPUT : FL_INT_INPUT);
        return input.handle(event);
    }
}

