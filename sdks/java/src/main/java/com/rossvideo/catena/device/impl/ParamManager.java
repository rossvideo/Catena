package com.rossvideo.catena.device.impl;

import com.rossvideo.catena.device.impl.params.DefaultParamManager.WidgetHint;

import catena.core.constraint.Constraint;
import catena.core.parameter.Param;
import catena.core.parameter.ParamType;
import catena.core.parameter.Value;

public interface ParamManager
{

    Param.Builder createOrGetParam(String oid);

    void clearParams();

    void commitChanges();

    Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value);

    Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value, Constraint constraint);

    Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value, Constraint constraint, WidgetHint widget);

    void addParamAlias(String fqoid, String alias);

    void addParamAliases(String fqoid, String[] aliases);

    Param.Builder setWidgetHint(Param.Builder param, WidgetHint widgetHint);

    Param.Builder setWidgetHint(Param.Builder param, String widgetHint);

    Value getValue(String oid, Integer index);

    void setValue(String oid, Integer index, Value value);

}