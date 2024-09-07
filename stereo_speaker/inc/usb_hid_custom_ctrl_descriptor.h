#ifndef USB_HID_CUSTOM_CTRL_DESCRIPTOR__H
#define USB_HID_CUSTOM_CTRL_DESCRIPTOR__H

#define USB_HID_SCAN_NEXT 0x01
#define USB_HID_SCAN_PREV 0x02
#define USB_HID_STOP      0x04
#define USB_HID_PAUSE     0x08
#define USB_HID_VOLUME    0x10
#define USB_HID_MUTE      0x20
#define USB_HID_VOL_UP    0x40
#define USB_HID_VOL_DEC   0x80

// Consumer Control Report Descriptor Template
#define MY_TUD_HID_REPORT_DESC_CONSUMER(...) \
		 HID_USAGE_PAGE ( HID_USAGE_PAGE_CONSUMER ),        /* Usage Page (Consumer)                                                          */\
		 HID_USAGE      ( HID_USAGE_CONSUMER_CONTROL ),     /* Usage (Consumer Control)                                                       */\
		 HID_COLLECTION ( HID_COLLECTION_APPLICATION ),     /* Collection (Application)                                                       */\
         /* Report ID if any */                                                                                                                 \
         __VA_ARGS__                                                                                                                            \
		 HID_USAGE_PAGE ( HID_USAGE_PAGE_CONSUMER ),        /*   Usage Page (Consumer)                                                        */\
		 HID_LOGICAL_MIN(0x00),                             /*   Logical Minimum (0)                                                          */\
		 HID_LOGICAL_MAX(0x01),                             /*   Logical Maximum (1)                                                          */\
		 HID_REPORT_SIZE(0x01),                             /*   Report Size (1)                                                              */\
		 HID_REPORT_COUNT(0x08),                            /*   Report Count (8)                                                             */\
		 HID_USAGE(HID_USAGE_CONSUMER_SCAN_NEXT            ),/*   Usage (Scan Next Track)                                                     */\
		 HID_USAGE(HID_USAGE_CONSUMER_SCAN_PREVIOUS        ),/*   Usage (Scan Previous Track)                                                 */\
		 HID_USAGE(HID_USAGE_CONSUMER_STOP                 ),/*   Usage (Stop)                                                                */\
		 HID_USAGE(HID_USAGE_CONSUMER_PLAY_PAUSE           ),/*   Usage (Play/Pause)                                                          */\
		 HID_USAGE(HID_USAGE_CONSUMER_VOLUME               ),/*   Usage (Volume)                                                              */\
		 HID_USAGE(HID_USAGE_CONSUMER_MUTE                 ),/*   Usage (Mute)                                                                */\
		 HID_USAGE(HID_USAGE_CONSUMER_VOLUME_INCREMENT     ),/*   Usage (Volume Increment)                                                    */\
		 HID_USAGE(HID_USAGE_CONSUMER_VOLUME_DECREMENT     ),/*   Usage (Volume Decrement)                                                    */\
         /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)        */                                                    \
		 HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_WRAP_NO | HID_LINEAR | HID_PREFERRED_STATE | HID_NO_NULL_POSITION ),                  \
		 HID_COLLECTION_END,              /* End Collection                                                                                   */\



#endif //USB_HID_CUSTOM_CTRL_DESCRIPTOR__H