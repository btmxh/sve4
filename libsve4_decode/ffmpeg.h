#pragma once

#include <stdint.h>

#include "sve4_decode_export.h"

#include "libsve4_log/api.h"
#include "libsve4_utils/defines.h"

#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavutil/fifo.h>
#include <tinycthread.h>

#include "decoder.h"
#include "error.h"
#include "ffmpeg_packet_queue.h"

SVE4_DECODE_EXPORT sve4_decode_error_t sve4_decode_ffmpeg_open_decoder(
    sve4_decode_decoder_t* _Nonnull decoder,
    const sve4_decode_decoder_config_t* _Nonnull config);
