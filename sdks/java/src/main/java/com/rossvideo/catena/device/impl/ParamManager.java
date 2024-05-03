package com.rossvideo.catena.device.impl;

import catena.core.constraint.Constraint;
import catena.core.parameter.Param;
import catena.core.parameter.ParamType;
import catena.core.parameter.Value;

public interface ParamManager
{
    public enum WidgetHint {
        DEFAULT,
        BUTTON,
        TEXT_DISPLAY,
        TEXT_ENTRY,
        HIDDEN,
        LABEL,
        TOGGLE,
        RADIO_BUTTONS,
        CHECKBOX,
        COLOR_CHOOSER,
        DOT,
        COMBO_BOX,
        LIST,
        DATE_PICKER,
        EQ_GRAPH,
        LINE_GRAPH,
        MULTI_LINE_TEXT,
        PASSWORD,
        SLIDER,
        FADER,
        WHEEL,
        SPINNER,
        TREE,
        JOYSTICK,
        PROGRESS,
        TABLE,
        FILE_CHOOSER        
    }
    
    Param.Builder createOrGetParam(String oid);
    
    public Param.Builder getParam(String oid);

    public void clearParams();

    public void commitChanges();
    
    public void commitChanges(String oid);

    public default Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value) {
        return createParamDescriptor(oid, name, type, readOnly, value, null);
    };

    public default Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value, Constraint constraint) {
        return createParamDescriptor(oid, name, type, readOnly, value, constraint, WidgetHint.DEFAULT);
    };

    public Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value, Constraint constraint, WidgetHint widget);

    public Param.Builder setWidgetHint(Param.Builder param, WidgetHint widgetHint);

    public Param.Builder setWidgetHint(Param.Builder param, String widgetHint);

    public Value getValue(String oid, Integer index);

    public void setValue(String oid, Integer index, Value value);

}