// buttons.c
#include "buttons.h"

#include "contextmenu.h"

#include "../flist.h"
#include "../friend.h"
#include "../groups.h"
#include "../screen_grab.h"
#include "../theme.h"

#include "../ui/svg.h"


/* buttons */
#ifdef UNITY
#include "xlib/mmenu.h"
extern bool unity_running;
#endif

/* Quick color change functions */
void button_setcolors_success(BUTTON *b) {
    b->c1  = COLOR_BTN_SUCCESS_BKGRND;
    b->c2  = COLOR_BTN_SUCCESS_BKGRND_HOVER;
    b->c3  = COLOR_BTN_SUCCESS_BKGRND_HOVER;
    b->ct1 = COLOR_BTN_SUCCESS_TEXT;
    b->ct2 = COLOR_BTN_SUCCESS_TEXT_HOVER;
}

void button_setcolors_danger(BUTTON *b) {
    b->c1  = COLOR_BTN_DANGER_BACKGROUND;
    b->c2  = COLOR_BTN_DANGER_BKGRND_HOVER;
    b->c3  = COLOR_BTN_DANGER_BKGRND_HOVER;
    b->ct1 = COLOR_BTN_DANGER_TEXT;
    b->ct2 = COLOR_BTN_DANGER_TEXT_HOVER;
}

void button_setcolors_warning(BUTTON *b) {
    b->c1  = COLOR_BTN_WARNING_BKGRND;
    b->c2  = COLOR_BTN_WARNING_BKGRND_HOVER;
    b->c3  = COLOR_BTN_WARNING_BKGRND_HOVER;
    b->ct1 = COLOR_BTN_WARNING_TEXT;
    b->ct2 = COLOR_BTN_WARNING_TEXT_HOVER;
}

void button_setcolors_disabled(BUTTON *b) {
    b->c1  = COLOR_BTN_DISABLED_BKGRND;
    b->c2  = COLOR_BTN_DISABLED_BKGRND;
    b->c3  = COLOR_BTN_DISABLED_BKGRND;
    b->ct1 = COLOR_BTN_DISABLED_TEXT;
    b->ct2 = COLOR_BTN_DISABLED_TEXT;
}

/* On-press functions followed by the update functions when needed... */
static void button_avatar_on_mup(void) {
    if (tox_thread_init) {
        openfileavatar();
    }
}

static void button_name_on_mup(void) {
    flist_selectsettings();
    panel_settings_profile.disabled = false;
    panel_settings_devices.disabled = true;
    panel_settings_net.disabled     = true;
    panel_settings_ui.disabled      = true;
    panel_settings_av.disabled      = true;
    edit_setfocus(&edit_name);
}

static void button_statusmsg_on_mup(void) {
    flist_selectsettings();
    panel_settings_profile.disabled = false;
    panel_settings_devices.disabled = true;
    panel_settings_net.disabled     = true;
    panel_settings_ui.disabled      = true;
    panel_settings_av.disabled      = true;
    edit_setfocus(&edit_status);
}

static void button_status_on_mup(void) {
    self.status++;
    if (self.status == 3) {
        self.status = 0;
    }

#ifdef UNITY
    if (unity_running) {
        mm_set_status(self.status);
    }
#endif

    postmessage_toxcore(TOX_SELF_SET_STATE, self.status, 0, NULL);
}

static void button_menu_update(BUTTON *b) {
    b->c1  = COLOR_BKGRND_MENU;
    b->c2  = COLOR_BKGRND_MENU_HOVER;
    b->c3  = COLOR_BKGRND_MENU_ACTIVE;
    b->ct1 = COLOR_MENU_TEXT;
    b->ct2 = COLOR_MENU_TEXT;
    if (b->mousedown || b->disabled) {
        b->ct1 = COLOR_MENU_TEXT_ACTIVE;
        b->ct2 = COLOR_MENU_TEXT_ACTIVE;
    }
    b->cd = COLOR_BKGRND_MENU_ACTIVE;
}

static void button_add_new_contact_on_mup(void) {
    if (tox_thread_init) {
        /* Only change if we're logged in! */
        edit_setstr(&edit_add_id, (char *)edit_search.data, edit_search.length);
        edit_setstr(&edit_search, (char *)"", 0);
        flist_selectaddfriend();
        edit_setfocus(&edit_add_msg);
    }
}

static void button_filter_friends_on_mup(void) {
    // this only works because right now there are only 2 filters
    // (none or online), basically a bool
    flist_set_filter(!flist_get_filter());
}

static void button_copyid_on_mup(void) {
    edit_setfocus(&edit_toxid);
    copy(0);
}

static void button_audiopreview_on_mup(void) {
    if (!settings.audio_preview) {
        postmessage_utoxav(UTOXAV_START_AUDIO, 1, 0, NULL);
    } else {
        postmessage_utoxav(UTOXAV_STOP_AUDIO, 1, 0, NULL);
    }
    settings.audio_preview = !settings.audio_preview;
}

static void button_audiopreview_update(BUTTON *b) {
    if (settings.audio_preview) {
        button_setcolors_danger(b);
    } else {
        button_setcolors_success(b);
    }
}

static void button_videopreview_on_mup(void) {
    if (settings.video_preview) {
        postmessage_utoxav(UTOXAV_STOP_VIDEO, 0, 1, NULL);
    } else if (video_width && video_height) {
        postmessage_utoxav(UTOXAV_START_VIDEO, 0, 1, NULL);
    } else {
        debug("Button ERR:\tVideo_width = 0, can't preview\n");
    }
    settings.video_preview = !settings.video_preview;
}

static void button_videopreview_update(BUTTON *b) {
    if (settings.video_preview) {
        button_setcolors_danger(b);
    } else {
        button_setcolors_success(b);
    }
}

static void button_send_friend_request_on_mup(void) {
    friend_add(edit_add_id.data, edit_add_id.length, edit_add_msg.data, edit_add_msg.length);
    edit_resetfocus();
}

static void button_group_audio_on_mup(void) {
    GROUPCHAT *g = flist_get_selected()->data;
    if (g->audio_calling) {
        postmessage_toxcore(TOX_GROUP_AUDIO_END, (g - group), 0, NULL);
    } else {
        postmessage_toxcore(TOX_GROUP_AUDIO_START, (g - group), 0, NULL);
    }
}

static void button_group_audio_update(BUTTON *b) {
    GROUPCHAT *g = flist_get_selected()->data;
    if (g->av_group) {
        b->disabled = false;
        if (g->audio_calling) {
            button_setcolors_danger(b);
        } else {
            button_setcolors_success(b);
        }
    } else {
        b->disabled = true;
        button_setcolors_disabled(b);
    }
}

static void button_call_decline_on_mup(void) {
    FRIEND *f = flist_get_selected()->data;
    if (f->call_state_friend) {
        debug("Declining call: %u\n", f->number);
        postmessage_toxcore(TOX_CALL_DISCONNECT, f->number, 0, NULL);
    }
}

static void button_call_decline_update(BUTTON *b) {
    FRIEND *f = flist_get_selected()->data;
    if (UTOX_AVAILABLE_AUDIO(f->number) && !UTOX_SENDING_AUDIO(f->number)) {
        button_setcolors_danger(b);
        b->nodraw   = false;
        b->disabled = false;
    } else {
        button_setcolors_disabled(b);
        b->nodraw   = true;
        b->disabled = true;
    }
}

static void button_call_audio_on_mup(void) {
    FRIEND *f = flist_get_selected()->data;
    if (f->call_state_self) {
        if (UTOX_SENDING_AUDIO(f->number)) {
            debug("Ending call: %u\n", f->number);
            /* var 3/4 = bool send video */
            postmessage_toxcore(TOX_CALL_DISCONNECT, f->number, 0, NULL);
        } else {
            debug("Canceling call: friend = %d\n", f->number);
            postmessage_toxcore(TOX_CALL_DISCONNECT, f->number, 0, NULL);
        }
    } else if (UTOX_AVAILABLE_AUDIO(f->number)) {
        debug("Accept Call: %u\n", f->number);
        postmessage_toxcore(TOX_CALL_ANSWER, f->number, 0, NULL);
    } else {
        if (f->online) {
            postmessage_toxcore(TOX_CALL_SEND, f->number, 0, NULL);
            debug("Calling friend: %u\n", f->number);
        }
    }
}

static void button_call_audio_update(BUTTON *b) {
    FRIEND *f = flist_get_selected()->data;
    if (UTOX_SENDING_AUDIO(f->number)) {
        button_setcolors_danger(b);
        b->disabled = false;
    } else if (UTOX_AVAILABLE_AUDIO(f->number)) {
        button_setcolors_warning(b);
        b->disabled = false;
    } else {
        if (f->online) {
            button_setcolors_success(b);
            b->disabled = false;
        } else {
            button_setcolors_disabled(b);
            b->disabled = true;
        }
    }
}

static void button_call_video_on_mup(void) {
    FRIEND *f = flist_get_selected()->data;
    if (f->call_state_self) {
        if (SELF_ACCEPT_VIDEO(f->number)) {
            debug("Canceling call (video): %u\n", f->number);
            postmessage_toxcore(TOX_CALL_PAUSE_VIDEO, f->number, 1, NULL);
        } else if (UTOX_SENDING_AUDIO(f->number)) {
            debug("Audio call inprogress, adding video\n");
            postmessage_toxcore(TOX_CALL_RESUME_VIDEO, f->number, 1, NULL);
        } else {
            debug("Ending call (video): %u\n", f->number);
            postmessage_toxcore(TOX_CALL_DISCONNECT, f->number, 1, NULL);
        }
    } else if (f->call_state_friend) {
        debug("Accept Call (video): %u %u\n", f->number, f->call_state_friend);
        postmessage_toxcore(TOX_CALL_ANSWER, f->number, 1, NULL);
    } else {
        if (f->online) {
            postmessage_toxcore(TOX_CALL_SEND, f->number, 1, NULL);
            debug("Calling friend (video): %u\n", f->number);
        }
    }
}

static void button_call_video_update(BUTTON *b) {
    FRIEND *f = flist_get_selected()->data;
    if (SELF_SEND_VIDEO(f->number)) {
        button_setcolors_danger(b);
        b->disabled = false;
    } else if (FRIEND_SENDING_VIDEO(f->number)) {
        button_setcolors_warning(b);
        b->disabled = false;
    } else {
        if (f->online) {
            button_setcolors_success(b);
            b->disabled = false;
        } else {
            button_setcolors_disabled(b);
            b->disabled = true;
        }
    }
}

static void button_accept_friend_on_mup(void) {
    FRIENDREQ *req = flist_get_selected()->data;
    postmessage_toxcore(TOX_FRIEND_ACCEPT, 0, 0, req);
    panel_friend_request.disabled = true;
}

static void contextmenu_avatar_onselect(uint8_t i) {
    if (i == 0) {
        avatar_unset_self();
    }
}

static void button_avatar_onright(void) {
    if (self_has_avatar()) {
        static UTOX_I18N_STR menu[] = { STR_REMOVE };
        contextmenu_new(countof(menu), menu, contextmenu_avatar_onselect);
    }
}

static void button_send_file_on_mup(void) {
    FRIEND *f = flist_get_selected()->data;
    if (f->online) {
        openfilesend();
    }
}

static void button_send_file_update(BUTTON *b) {
    FRIEND *f = flist_get_selected()->data;
    if (f->online) {
        b->disabled = false;
        button_setcolors_success(b);
    } else {
        b->disabled = true;
        button_setcolors_disabled(b);
    }
}

static void button_send_screenshot_on_mup(void) {
    FRIEND *f = flist_get_selected()->data;
    if (f->online) {
        utox_screen_grab_desktop(0);
    }
}

static void button_send_screenshot_update(BUTTON *b) {
    FRIEND *f = flist_get_selected()->data;
    if (f->online) {
        b->disabled = false;
        button_setcolors_success(b);
    } else {
        b->disabled = true;
        button_setcolors_disabled(b);
    }
}

/* Button to send chat message */
static void button_chat_send_on_mup(void) {
    if (flist_get_selected()->item == ITEM_FRIEND) {
        FRIEND *f = flist_get_selected()->data;
        if (f->online) {
            // TODO clear the chat bar with a /slash command
            edit_msg_onenter(&edit_msg);
            // reset focus to the chat window on send to prevent segfault. May break on android.
            edit_setfocus(&edit_msg);
        }
    } else {
        edit_msg_onenter(&edit_msg_group);
        // reset focus to the chat window on send to prevent segfault. May break on android.
        edit_setfocus(&edit_msg_group);
    }
}

static void button_chat_send_update(BUTTON *b) {
    if (flist_get_selected()->item == ITEM_FRIEND) {
        FRIEND *f = flist_get_selected()->data;
        if (f->online) {
            b->disabled = false;
            button_setcolors_success(b);
        } else {
            b->disabled = true;
            button_setcolors_disabled(b);
        }
    } else {
        b->disabled = false;
        button_setcolors_success(b);
    }
}

static void button_lock_uTox_on_mup(void) {
    if (tox_thread_init && edit_profile_password.length > 3) {
        flist_selectsettings();
        panel_profile_password.disabled = false;
        panel_settings_master.disabled  = true;
        tox_settingschanged();
    }
}

static void button_show_password_settings_on_mup(void) {
    button_show_password_settings.disabled = true;
    button_show_password_settings.nodraw = true;
    panel_profile_password_settings.disabled = false;
}

static void button_export_chatlog_on_mup(void) {
    utox_export_chatlog_init(((FRIEND *)flist_get_selected()->data)->number);
}


BUTTON button_avatar = {
    .nodraw = true, .on_mup = button_avatar_on_mup, .onright = button_avatar_onright,
};

BUTTON button_name = {
    .nodraw = true, .on_mup = button_name_on_mup,
};

BUTTON button_status_msg = {
    .nodraw = true, .on_mup = button_statusmsg_on_mup,
};

BUTTON button_usr_state = {
    .nodraw = true, .on_mup = button_status_on_mup, .tooltip_text = {.i18nal = STR_STATUS },
};

BUTTON button_filter_friends = {
    .nodraw = true,
    .on_mup = button_filter_friends_on_mup,
    .tooltip_text = {.i18nal = STR_FILTER_CONTACT_TOGGLE },
};

BUTTON button_add_new_contact = {
    .bm2          = BM_ADD,
    .bw           = _BM_ADD_WIDTH,
    .bh           = _BM_ADD_WIDTH,
    .update       = button_menu_update,
    .on_mup      = button_add_new_contact_on_mup,
    .disabled     = true,
    .nodraw       = true,
    .tooltip_text = {.i18nal = STR_ADDFRIENDS },
};

BUTTON button_copyid = {
    .bm          = BM_SBUTTON,
    .button_text = {.i18nal = STR_COPY_TOX_ID },
    .update   = button_setcolors_success,
    .on_mup  = button_copyid_on_mup,
    .disabled = false,
};

BUTTON button_send_friend_request = {
    .bm          = BM_SBUTTON,
    .button_text = {.i18nal = STR_ADD },
    .update   = button_setcolors_success,
    .on_mup  = button_send_friend_request_on_mup,
    .disabled = false,
};

BUTTON button_call_decline = {
    .bm           = BM_LBUTTON,
    .bm2          = BM_DECLINE,
    .bw           = _BM_LBICON_WIDTH,
    .bh           = _BM_LBICON_HEIGHT,
    .on_mup      = button_call_decline_on_mup,
    .update       = button_call_decline_update,
    .tooltip_text = {.i18nal = STR_CALL_DECLINE },
    .nodraw   = true,
    .disabled = true,
};

BUTTON button_call_audio = {
    .bm           = BM_LBUTTON,
    .bm2          = BM_CALL,
    .bw           = _BM_LBICON_WIDTH,
    .bh           = _BM_LBICON_HEIGHT,
    .on_mup      = button_call_audio_on_mup,
    .update       = button_call_audio_update,
    .tooltip_text = {.i18nal = STR_CALL_START_AUDIO },
};

BUTTON button_call_video = {
    .bm           = BM_LBUTTON,
    .bm2          = BM_VIDEO,
    .bw           = _BM_LBICON_WIDTH,
    .bh           = _BM_LBICON_HEIGHT,
    .on_mup      = button_call_video_on_mup,
    .update       = button_call_video_update,
    .tooltip_text = {.i18nal = STR_CALL_START_VIDEO },
};

BUTTON button_group_audio = {
    .bm           = BM_LBUTTON,
    .bm2          = BM_CALL,
    .bw           = _BM_LBICON_WIDTH,
    .bh           = _BM_LBICON_HEIGHT,
    .on_mup      = button_group_audio_on_mup,
    .update       = button_group_audio_update,
    .tooltip_text = {.i18nal = STR_GROUPCHAT_JOIN_AUDIO },
};

BUTTON button_accept_friend = {
    .bm          = BM_SBUTTON,
    .button_text = {.i18nal = STR_ADD },
    .update  = button_setcolors_success,
    .on_mup = button_accept_friend_on_mup,
};

BUTTON button_callpreview = {
    .bm       = BM_LBUTTON,
    .bm2      = BM_CALL,
    .bw       = _BM_LBICON_WIDTH,
    .bh       = _BM_LBICON_HEIGHT,
    .on_mup  = button_audiopreview_on_mup,
    .update   = button_audiopreview_update,
    .disabled = false,
};

BUTTON button_videopreview = {
    .bm       = BM_LBUTTON,
    .bm2      = BM_VIDEO,
    .bw       = _BM_LBICON_WIDTH,
    .bh       = _BM_LBICON_HEIGHT,
    .on_mup  = button_videopreview_on_mup,
    .update   = button_videopreview_update,
    .disabled = false,
};

BUTTON button_send_file = {
    .bm           = BM_CHAT_BUTTON_LEFT,
    .bm2          = BM_FILE,
    .bw           = _BM_FILE_WIDTH,
    .bh           = _BM_FILE_HEIGHT,
    .on_mup      = button_send_file_on_mup,
    .update       = button_send_file_update,
    .disabled     = true,
    .tooltip_text = {.i18nal = STR_SEND_FILE },
};

BUTTON button_send_screenshot = {
    .bm           = BM_CHAT_BUTTON_RIGHT,
    .bm2          = BM_CHAT_BUTTON_OVERLAY_SCREENSHOT,
    .bw           = _BM_CHAT_BUTTON_OVERLAY_WIDTH,
    .bh           = _BM_CHAT_BUTTON_OVERLAY_HEIGHT,
    .update       = button_send_screenshot_update,
    .on_mup      = button_send_screenshot_on_mup,
    .tooltip_text = {.i18nal = STR_SENDSCREENSHOT },
};

BUTTON button_chat_send = {
    .bm           = BM_CHAT_SEND,
    .bm2          = BM_CHAT_SEND_OVERLAY,
    .bw           = _BM_CHAT_SEND_OVERLAY_WIDTH,
    .bh           = _BM_CHAT_SEND_OVERLAY_HEIGHT,
    .on_mup      = button_chat_send_on_mup,
    .update       = button_chat_send_update,
    .tooltip_text = {.i18nal = STR_SENDMESSAGE },
};

BUTTON button_lock_uTox = {
    .bm          = BM_SBUTTON,
    .update      = button_setcolors_success,
    .on_mup     = button_lock_uTox_on_mup,
    .button_text = {.i18nal = STR_LOCK },
    .tooltip_text = {.i18nal = STR_LOCK_UTOX },
};

BUTTON button_show_password_settings = {
    .bm          = BM_SBUTTON,
    .update      = button_setcolors_success,
    .on_mup     = button_show_password_settings_on_mup,
    .button_text = {.i18nal = STR_SHOW },
    .tooltip_text = {.i18nal = STR_SHOW_UI_PASSWORD },
};

BUTTON button_export_chatlog = {
    .bm          = BM_SBUTTON,
    .button_text = {.i18nal = STR_FRIEND_EXPORT_CHATLOG },
    .update   = button_setcolors_success,
    .on_mup  = button_export_chatlog_on_mup,
    .disabled = false,
};

extern SCROLLABLE scrollbar_settings;

static void button_settings_on_mup(void) {
    if (tox_thread_init) {
        flist_selectsettings();
    }
}

static void disable_all_setting_sub(void) {
    flist_selectsettings();
    panel_settings_profile.disabled = true;
    panel_settings_devices.disabled = true;
    panel_settings_net.disabled     = true;
    panel_settings_ui.disabled      = true;
    panel_settings_av.disabled      = true;
}

static void button_settings_sub_profile_on_mup(void) {
    scrollbar_settings.content_height = SCALE(260);
    disable_all_setting_sub();
    panel_settings_profile.disabled = false;
    button_show_password_settings.disabled = false;
    button_show_password_settings.nodraw = false;
}

static void button_settings_sub_devices_on_mup(void) {
    scrollbar_settings.content_height = SCALE(260);
    disable_all_setting_sub();
    panel_settings_devices.disabled = false;
}

static void button_settings_sub_net_on_mup(void) {
    scrollbar_settings.content_height = SCALE(180);
    disable_all_setting_sub();
    panel_settings_net.disabled = false;
}

static void button_settings_sub_ui_on_mup(void) {
    scrollbar_settings.content_height = SCALE(280);
    disable_all_setting_sub();
    panel_settings_ui.disabled = false;
}

static void button_settings_sub_av_on_mup(void) {
    scrollbar_settings.content_height = SCALE(300);
    disable_all_setting_sub();
    panel_settings_av.disabled = false;
}

static void button_bottommenu_update(BUTTON *b) {
    b->c1  = COLOR_BKGRND_MENU;
    b->c2  = COLOR_BKGRND_MENU_HOVER;
    b->c3  = COLOR_BKGRND_MENU_ACTIVE;
    b->ct1 = COLOR_MENU_TEXT;
    b->ct2 = COLOR_MENU_TEXT;
    if (b->mousedown || b->disabled) {
        b->ct1 = COLOR_MENU_TEXT_ACTIVE;
        b->ct2 = COLOR_MENU_TEXT_ACTIVE;
    }
    b->cd = COLOR_BKGRND_MENU_ACTIVE;
}

static void button_add_device_to_self_mdown(void) {
#ifdef ENABLE_MULTIDEVICE
    devices_self_add(edit_add_new_device_to_self.data, edit_add_new_device_to_self.length);
    edit_resetfocus();
#endif
}

BUTTON
    button_settings = {
        .bm2          = BM_SETTINGS,
        .bw           = _BM_ADD_WIDTH,
        .bh           = _BM_ADD_WIDTH,
        .update       = button_bottommenu_update,
        .on_mup      = button_settings_on_mup,
        .disabled     = false,
        .nodraw       = false,
        .tooltip_text = {.i18nal = STR_USERSETTINGS },
    },

    button_settings_sub_profile = {
        .nodraw = true,
        .on_mup = button_settings_sub_profile_on_mup,
        .tooltip_text = {.i18nal = STR_UTOX_SETTINGS },
    },

    button_settings_sub_devices = {
        .nodraw = true,
        .on_mup = button_settings_sub_devices_on_mup,
        .tooltip_text = {.i18nal = STR_UTOX_SETTINGS },
    },

    button_settings_sub_net = {
        .nodraw = true,
        .on_mup = button_settings_sub_net_on_mup,
        .tooltip_text = {.i18nal = STR_NETWORK_SETTINGS },
    },

    button_settings_sub_ui = {
        .nodraw = true,
        .on_mup = button_settings_sub_ui_on_mup,
        .tooltip_text = {.i18nal = STR_USERSETTINGS },
    },

    button_settings_sub_av = {
        .nodraw = true,
        .on_mup = button_settings_sub_av_on_mup,
        .tooltip_text = {.i18nal = STR_AUDIO_VIDEO },
    },

    button_add_new_device_to_self = {
        .bm          = BM_SBUTTON,
        .button_text = {.i18nal = STR_ADD },
        .update  = button_setcolors_success,
        .on_mup = button_add_device_to_self_mdown,
    };

static void btn_move_window_mdn(void) {
    debug("button move down\n");
    btn_move_window_down = true;
}

static void btn_move_window_mup(void) {
    debug("button move up\n");
    btn_move_window_down = false;
}

BUTTON button_move_window = {
    .nodraw   = true,
    .disabled = true,
    .on_mdn   = btn_move_window_mdn,
    .on_mup   = btn_move_window_mup,
};

static void btn_test_notify_mup(void) {
    debug("button test\n");

    native_notify_new();

}

BUTTON button_test_notify = {
    .nodraw   = false,
    .disabled = false,
    .on_mup   = btn_test_notify_mup,
};
