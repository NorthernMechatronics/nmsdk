#ifndef __LORAMAC_COMPLIANCE_H__
#define __LORAMAC_COMPLIANCE_H__

#define LORAMAC_COMPLIANCE_TEST_PORT 224

typedef enum
{
    CHECK_DISABLE_CMD = 0, // (ii)
    STATE_1,               // (iii, iv)
    ENABLE_CONFIRMATION,   // (v)
    DISABLE_CONFIRMATION,  // (vi)
    STATE_4,               // (vii)
    LINK_CHECK,            // (viii)
    EXIT_TEST,             // (ix)
    TXCW,                  // (x)
    REQ_DEVICE_TIME,       // (Send Device Time Request)
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
