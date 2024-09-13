#include "usb_microphone.h"
#include "common_types.h"
#include "i2s_board_defines.h"
#include "volume_ctrl.h"

//----------------------------------------
// local variables:
//----------------------------------------

// Audio controls
// Current states
bool mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];                  // +1 for master channel 0
uint16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];          // +1 for master channel 0
uint32_t sampFreq;
uint8_t bytesPerSample;
uint8_t clkValid;

// Range states
// List of supported sample rates
static const uint32_t sampleRatesList[] =
{
    16000, 32000, 48000
};

#define N_sampleRates  TU_ARRAY_SIZE(sampleRatesList)

// Bytes per format of every Alt settings
static const uint8_t bytesPerSampleAltList[CFG_TUD_AUDIO_FUNC_1_N_FORMATS] =
{
    CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX,
    CFG_TUD_AUDIO_FUNC_1_FORMAT_2_N_BYTES_PER_SAMPLE_TX,
};

// Resolution per format
const uint8_t resolutions_per_format[CFG_TUD_AUDIO_FUNC_1_N_FORMATS] = {CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX,
                                                                        CFG_TUD_AUDIO_FUNC_1_FORMAT_2_RESOLUTION_RX};

audio_control_range_2_n_t(1) volumeRng[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX+1];       // Volume range state

//----------------------------------------
// Functions declared in the header:
//----------------------------------------
void usb_microphone_init()
{
  board_init();

	// init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  // Init values
  sampFreq = I2S_MIC_RATE_DEF;
  clkValid = 1;

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1); i++)
  {
    volume[i] = DEFAULT_VOLUME;
    mute[i] = 0;
  }
}

static usb_microphone_mute_set_cb_t usb_microphone_mute_set_handler = NULL;
static usb_microphone_volume_set_cb_t usb_microphone_volume_set_handler = NULL;
static usb_microphone_current_sample_rate_set_cb_t usb_microphone_current_sample_rate_set_handler = NULL;
static usb_microphone_current_resolution_set_cb_t usb_microphone_current_resolution_set_handler = NULL;
static usb_microphone_current_status_set_cb_t usb_microphone_current_status_set_handler = NULL;
static usb_microphone_tx_pre_load_cb_t usb_microphone_tx_pre_load_handler = NULL;
static usb_microphone_tx_post_load_cb_t usb_microphone_tx_post_load_handler = NULL;

void usb_microphone_set_mute_set_handler(usb_microphone_mute_set_cb_t handler){
  usb_microphone_mute_set_handler = handler;
}
void usb_microphone_set_volume_set_handler(usb_microphone_volume_set_cb_t handler){
  usb_microphone_volume_set_handler = handler;
}

void usb_microphone_set_current_sample_rate_set_handler(usb_microphone_current_sample_rate_set_cb_t handler){
  usb_microphone_current_sample_rate_set_handler = handler;
}

void usb_microphone_set_current_resolution_set_handler(usb_microphone_current_resolution_set_cb_t handler){
  usb_microphone_current_resolution_set_handler = handler;
}

void usb_microphone_set_current_status_set_handler(usb_microphone_current_status_set_cb_t handler){
  usb_microphone_current_status_set_handler = handler;
}

void usb_microphone_set_tx_pre_load_handler(usb_microphone_tx_pre_load_cb_t handler){
	usb_microphone_tx_pre_load_handler = handler;
}

void usb_microphone_set_tx_post_load_handler(usb_microphone_tx_post_load_cb_t handler){
	usb_microphone_tx_post_load_handler = handler;
}

void usb_microphone_task()
{
	tud_task(); // tinyusb device task
}

//----------------------------------------
//----------------------------------------

//----------------------------------------
// Tiny USB functions::
//----------------------------------------

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  if(usb_microphone_current_status_set_handler){
    usb_microphone_current_status_set_handler(BLINK_MOUNTED);
  }
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  if(usb_microphone_current_status_set_handler){
    usb_microphone_current_status_set_handler(BLINK_NOT_MOUNTED);
  }
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
  if(usb_microphone_current_status_set_handler){
    usb_microphone_current_status_set_handler(BLINK_SUSPENDED);
  }
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  if(usb_microphone_current_status_set_handler){
    usb_microphone_current_status_set_handler(tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED);
  }
}

//--------------------------------------------------------------------+
// Application Callback API Implementations
//--------------------------------------------------------------------+

// Invoked when set interface is called, typically on start/stop streaming or format change
bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
  (void)rhport;
  uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
  uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

  TU_LOG2("Set interface %d alt %d\r\n", itf, alt);
  if (ITF_NUM_AUDIO_STREAMING == itf && alt != 0)
  {
    if(usb_microphone_current_status_set_handler){
      usb_microphone_current_status_set_handler(BLINK_STREAMING);
    }
  }

  // Clear buffer when streaming format is changed
  if(alt != 0)
  {
    bytesPerSample = bytesPerSampleAltList[alt-1];
    if(usb_microphone_current_resolution_set_handler){
      usb_microphone_current_resolution_set_handler(resolutions_per_format[alt-1]);
    }
  }
  return true;
}

// Invoked when audio class specific set request received for an EP
bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{
  (void) rhport;
  (void) pBuff;

  // We do not support any set range requests here, only current value requests
  TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

  // Page 91 in UAC2 specification
  uint8_t channelNum = TU_U16_LOW(p_request->wValue);
  uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
  uint8_t ep = TU_U16_LOW(p_request->wIndex);

  (void) channelNum; (void) ctrlSel; (void) ep;

  return false;   // Yet not implemented
}

// Invoked when audio class specific set request received for an interface
bool tud_audio_set_req_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{
  (void) rhport;
  (void) pBuff;

  // We do not support any set range requests here, only current value requests
  TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

  // Page 91 in UAC2 specification
  uint8_t channelNum = TU_U16_LOW(p_request->wValue);
  uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
  uint8_t itf = TU_U16_LOW(p_request->wIndex);

  (void) channelNum; (void) ctrlSel; (void) itf;

  return false;   // Yet not implemented
}

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{
  (void) rhport;

  // Page 91 in UAC2 specification
  uint8_t channelNum = TU_U16_LOW(p_request->wValue);
  uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
  uint8_t itf = TU_U16_LOW(p_request->wIndex);
  uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

  (void) itf;

  // We do not support any set range requests here, only current value requests
  TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

  // If request is for our feature unit
  if ( entityID == UAC2_ENTITY_FEATURE_UNIT )
  {
    switch ( ctrlSel )
    {
      case AUDIO_FU_CTRL_MUTE:
        // Request uses format layout 1
        TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_1_t));

        mute[channelNum] = ((audio_control_cur_1_t*) pBuff)->bCur;
        if(usb_microphone_mute_set_handler)
        {
          usb_microphone_mute_set_handler(channelNum, mute[channelNum]);
        }

        TU_LOG2("    Set Mute: %d of channel: %u\r\n", mute[channelNum], channelNum);
      return true;

      case AUDIO_FU_CTRL_VOLUME:
        // Request uses format layout 2
        TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_2_t));

        if(usb_microphone_volume_set_handler)
        {
          usb_microphone_volume_set_handler(channelNum, volume[channelNum]);
        }
        volume[channelNum] = (uint16_t) ((audio_control_cur_2_t*) pBuff)->bCur;

        TU_LOG2("    Set Volume: %d dB of channel: %u\r\n", volume[channelNum], channelNum);
      return true;

        // Unknown/Unsupported control
      default:
        TU_BREAKPOINT();
      return false;
    }
  }

  // Clock Source unit
  if ( entityID == UAC2_ENTITY_CLOCK )
  {
    switch ( ctrlSel )
    {
      case AUDIO_CS_CTRL_SAM_FREQ:
        TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_4_t));

        sampFreq = (uint32_t)((audio_control_cur_4_t *)pBuff)->bCur;
        if(usb_microphone_current_sample_rate_set_handler){
          usb_microphone_current_sample_rate_set_handler(sampFreq);
        }

        TU_LOG2("Clock set current freq: %" PRIu32 "\r\n", sampFreq);

        return true;
      break;

      // Unknown/Unsupported control
      default:
        TU_BREAKPOINT();
        return false;
    }
  }

  return false;    // Yet not implemented
}

// Invoked when audio class specific get request received for an EP
bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
  (void) rhport;

  // Page 91 in UAC2 specification
  uint8_t channelNum = TU_U16_LOW(p_request->wValue);
  uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
  uint8_t ep = TU_U16_LOW(p_request->wIndex);

  (void) channelNum; (void) ctrlSel; (void) ep;

  //  return tud_control_xfer(rhport, p_request, &tmp, 1);

  return false;   // Yet not implemented
}

// Invoked when audio class specific get request received for an interface
bool tud_audio_get_req_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
  (void) rhport;

  // Page 91 in UAC2 specification
  uint8_t channelNum = TU_U16_LOW(p_request->wValue);
  uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
  uint8_t itf = TU_U16_LOW(p_request->wIndex);

  (void) channelNum; (void) ctrlSel; (void) itf;

  return false;   // Yet not implemented
}

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
  (void) rhport;

  // Page 91 in UAC2 specification
  uint8_t channelNum = TU_U16_LOW(p_request->wValue);
  uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
  // uint8_t itf = TU_U16_LOW(p_request->wIndex);       // Since we have only one audio function implemented, we do not need the itf value
  uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

  // Input terminal (Microphone input)
  if (entityID == UAC2_ENTITY_INPUT_TERMINAL)
  {
    switch ( ctrlSel )
    {
      case AUDIO_TE_CTRL_CONNECTOR:
      {
        // The terminal connector control only has a get request with only the CUR attribute.
        audio_desc_channel_cluster_t ret;

        // Those are dummy values for now
        ret.bNrChannels = 1;
        ret.bmChannelConfig = 0;
        ret.iChannelNames = 0;

        TU_LOG2("    Get terminal connector\r\n");

        return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void*) &ret, sizeof(ret));
      }
      break;

        // Unknown/Unsupported control selector
      default:
        TU_BREAKPOINT();
        return false;
    }
  }

  // Feature unit
  if (entityID == UAC2_ENTITY_FEATURE_UNIT)
  {
    switch ( ctrlSel )
    {
      case AUDIO_FU_CTRL_MUTE:
        // Audio control mute cur parameter block consists of only one byte - we thus can send it right away
        // There does not exist a range parameter block for mute
        TU_LOG2("    Get Mute of channel: %u\r\n", channelNum);
        return tud_control_xfer(rhport, p_request, &mute[channelNum], 1);

      case AUDIO_FU_CTRL_VOLUME:
        switch ( p_request->bRequest )
        {
          case AUDIO_CS_REQ_CUR:
            TU_LOG2("    Get Volume of channel: %u\r\n", channelNum);
            return tud_control_xfer(rhport, p_request, &volume[channelNum], sizeof(volume[channelNum]));

          case AUDIO_CS_REQ_RANGE:
            TU_LOG2("    Get Volume range of channel: %u\r\n", channelNum);

            // Copy values - only for testing - better is version below
            audio_control_range_2_n_t(1) ret;

            ret.wNumSubRanges = 1;
            ret.subrange[0].bMin = MIN_VOLUME;    // -90 dB
            ret.subrange[0].bMax = MAX_VOLUME;    // +90 dB
            ret.subrange[0].bRes = VOLUME_RESOLUTION;     // 1 dB steps

            return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void*) &ret, sizeof(ret));

            // Unknown/Unsupported control
          default:
            TU_BREAKPOINT();
            return false;
        }
      break;

        // Unknown/Unsupported control
      default:
        TU_BREAKPOINT();
        return false;
    }
  }

  // Clock Source unit
  if ( entityID == UAC2_ENTITY_CLOCK )
  {
    switch ( ctrlSel )
    {
      case AUDIO_CS_CTRL_SAM_FREQ:
        // channelNum is always zero in this case
        switch ( p_request->bRequest )
        {
          case AUDIO_CS_REQ_CUR:
            TU_LOG2("    Get Sample Freq.\r\n");
            return tud_control_xfer(rhport, p_request, &sampFreq, sizeof(sampFreq));

          case AUDIO_CS_REQ_RANGE:
          {
            TU_LOG2("    Get Sample Freq. range\r\n");
            audio_control_range_4_n_t(N_sampleRates) rangef =
            {
                .wNumSubRanges = tu_htole16(N_sampleRates)
            };
            TU_LOG1("Clock get %d freq ranges\r\n", N_sampleRates);
            for(uint8_t i = 0; i < N_sampleRates; i++)
            {
                rangef.subrange[i].bMin = (int32_t)sampleRatesList[i];
                rangef.subrange[i].bMax = (int32_t)sampleRatesList[i];
                rangef.subrange[i].bRes = 0;
                TU_LOG1("Range %d (%d, %d, %d)\r\n", i, (int)rangef.subrange[i].bMin, (int)rangef.subrange[i].bMax, (int)rangef.subrange[i].bRes);
            }
            return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &rangef, sizeof(rangef));
          }
           // Unknown/Unsupported control
          default:
            TU_BREAKPOINT();
            return false;
        }
      break;

      case AUDIO_CS_CTRL_CLK_VALID:
        // Only cur attribute exists for this request
        TU_LOG2("    Get Sample Freq. valid\r\n");
        return tud_control_xfer(rhport, p_request, &clkValid, sizeof(clkValid));

      // Unknown/Unsupported control
      default:
        TU_BREAKPOINT();
        return false;
    }
  }

  TU_LOG2("  Unsupported entity: %d\r\n", entityID);
  return false;   // Yet not implemented
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  (void) rhport;
  (void) itf;
  (void) ep_in;
  (void) cur_alt_setting;

  if (usb_microphone_tx_pre_load_handler)
  {
    usb_microphone_tx_pre_load_handler(rhport, itf, ep_in, cur_alt_setting);
  }

  return true;
}

bool tud_audio_tx_done_post_load_cb(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  (void) rhport;
  (void) n_bytes_copied;
  (void) itf;
  (void) ep_in;
  (void) cur_alt_setting;

  if (usb_microphone_tx_post_load_handler)
  {
    usb_microphone_tx_post_load_handler(rhport, n_bytes_copied, itf, ep_in, cur_alt_setting);
  }

  return true;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
  (void) rhport;
  (void) p_request;

  return true;
}

//----------------------------------------
//----------------------------------------