
#define AC_OFFLINE  0x00
#define AC_ONLINE   0x01
#define AC_ONBACKUP 0x02

#define BATTERY_VOLT_HIGH     0x01
#define BATTERY_VOLT_LOW      0x02
#define BATTERY_VOLT_CRITICAL 0x04
#define BATTERY_CHARGING      0x08
#define BATTERY_NO_SELECT     0x10
#define BATTERY_NOT_AVAIL     0x80

typedef struct {
  uint8 ac_status;
  uint8 battery_status;
  uint8 battery_flag;
  uint8 battery_percent;
  uint32 remain_seconds;
} POWER_STATUS;

