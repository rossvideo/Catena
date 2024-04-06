package com.rossvideo.catena.device.impl;

import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.Map;

import catena.core.device.Device;
import catena.core.menu.Menu;
import catena.core.menu.MenuGroup;

public class MenuGroupManager
{
    private Map<String, Menu.Builder> menus = new LinkedHashMap<>();
    private Map<String, MenuGroup.Builder> menuGroups = new LinkedHashMap<>();
    private Device.Builder deviceBuilder;
    
    public MenuGroupManager(Device.Builder deviceBuilder) {
        this.deviceBuilder = deviceBuilder;
    }
    
    protected MenuGroup.Builder createOrGetMenuGroup(String groupId)
    {
        MenuGroup.Builder groupBuilder = menuGroups.get(groupId);
        if (groupBuilder == null)
        {
            groupBuilder = deviceBuilder.putMenuGroupsBuilderIfAbsent(groupId);
            menuGroups.put(groupId, groupBuilder);
        }

        return groupBuilder;
    }
    
    public void createMenuGroup(String groupId, String name) {
        createOrGetMenuGroup(groupId)
                .setName(TextUtils.createSimpleText(name));
    }
    
    protected Menu.Builder createOrGetMenu(String groupId, String menuId) {
        Menu.Builder menuBuilder = menus.get(groupId + "/" + menuId);
        if (menuBuilder != null)
        {
            return menuBuilder;
        }
        
        MenuGroup.Builder groupBuilder = createOrGetMenuGroup(groupId);
        menuBuilder = groupBuilder.putMenusBuilderIfAbsent(menuId);
        menus.put(groupId + "/" + menuId, menuBuilder);
        
        return menuBuilder;
    }
    
    public void createMenu(String groupId, String menuId, String name) {
        createOrGetMenu(groupId, menuId)
                .setName(TextUtils.createSimpleText(name));
    }
    
    public void addParamsMenu(String groupId, String menuId, String oid) {
        addParamsMenu(groupId, menuId, new String[] { oid });
    }
    
    public void addParamsMenu(String groupId, String menuId, String[] oid) { 
        createOrGetMenu(groupId, menuId).addAllParamOids(Arrays.asList(oid));
            
    }
}
