#ifndef __LORAMAC_COMPLIANCE_H__
#define __LORAMAC_COMPLIANCE_H__

#define LORAMAC_COMPLIANCE_TEST_PORT 224

typedef enum
{
    COMPLIANCE_EXIT = 0,        // (ii)
    COMPLIANCE_ACTIVE,          // (iii, iv)
    COMPLIANCE_ACK_ENABLE,      // (v)
    COMPLIANCE_ACK_DISABLE,     // (vi)
    COMPLIANCE_RX,              // (vii)
    COMPLIANCE_LINK_CHECK,      // (viii)
    COMPLIANCE_EXIT_AND_REJOIN, // (ix)
    COMPLIANCE_TXCW,            // (x)
    COMPLIANCE_REQ_DEVICE_TIME, // (Send Device Time Request)
	COMPLIANCE_SWITCH_CLASS,
	COMPLIANCE_REQ_PING_SLOT_INFO,
	COMPLIANCE_REQ_BEACON_TIMING,
} loramac_compliance_state_e;

typedef struct
{
    bool Running;
    uint8_t State;
    bool IsTxConfirmed;
    uint8_t AppPort;
    uint8_t AppDataSize;
    uint8_t AppDataBuffer[MAX_APP_DATA_SIZE];
    uint16_t DownLinkCounter;
    bool LinkCheck;
    uint8_t DemodMargin;
    uint8_t NbGateways;
} loramac_compliance_status_t;

#endif
