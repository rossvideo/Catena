"use strict";


class LanguagePacks {
    constructor(desc){
        this.desc = desc;
    }

    getLanguagePacks(){
        let ans = `{}`;
        if ("language_packs" in this.desc) {
            ans = this.desc.language_packs.packs;
        }
        return ans;
    }
}

module.exports = LanguagePacks;


