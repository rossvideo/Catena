package com.rossvideo.catena.device.impl;

import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.Map;

import catena.core.menu.Menu;
import catena.core.menu.MenuGroup;
import catena.core.menu.MenuGroups;

public class MenuGroupManager
{
    private Map<String, Menu.Builder> menus = new LinkedHashMap<>();
    private Map<String, MenuGroup.Builder> menuGroups = new LinkedHashMap<>();
    private MenuGroups.Builder menuGroupsBuilder;
    
    public MenuGroupManager(MenuGroups.Builder menuGroupsBuilder) {
        this.menuGroupsBuilder = menuGroupsBuilder;
    }
    
    public MenuGroups buildMenuGroups()
    {
        return menuGroupsBuilder.build();
    }
    
    protected MenuGroup.Builder createOrGetMenuGroup(String groupId)
    {
        MenuGroup.Builder groupBuilder = menuGroups.get(groupId);
        if (groupBuilder == null)
        {
            groupBuilder = menuGroupsBuilder.putMenuGroupsBuilderIfAbsent(groupId);
            menuGroups.put(groupId, groupBuilder);
        }

        return groupBuilder;
    }
    
    public void createMenuGroup(String groupId, int numericID, String name) {
        createOrGetMenuGroup(groupId)
                .setId(numericID)
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
    
    public void createMenu(String groupId, String menuId, int numericID, String name) {
        createOrGetMenu(groupId, menuId)
                .setId(numericID)
                .setName(TextUtils.createSimpleText(name));
    }
    
    public void addParamsMenu(String groupId, String menuId, String oid) {
        addParamsMenu(groupId, menuId, new String[] { oid });
    }
    
    public void addParamsMenu(String groupId, String menuId, String[] oid) { 
        createOrGetMenu(groupId, menuId).addAllParamOids(Arrays.asList(oid));
            
    }
}
