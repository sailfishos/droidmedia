#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/MediaDefs.h>
#include "droidmediacodec.h"

static void mpeg4video(android::sp<android::MetaData> md, void *codec_data, ssize_t codec_data_size) {
    // TODO:
    // This code is crap and it was ripped off android's MPEG4 writer
    uint8_t data[codec_data_size + 29];
    data[0] = data[1] = data[2] = data[3] = 0x0;
    data[4] = 0x03;
    data[5] = 23 + codec_data_size;
    data[6] = data[7] = 0x0;
    data[8] = 0x1f;
    data[9] = 0x04;
    data[10] = 15 + codec_data_size;
    data[11] = 0x20;
    data[12] = 0x11;
    data[13] = 0x01;
    data[14] = 0x77;
    data[15] = 0x00;
    data[16] = 0x00;
    data[17] = 0x03;
    data[18] = 0xe8;
    data[19] = 0x00;
    data[20] = 0x00;
    data[21] = 0x03;
    data[22] = 0xe8;
    data[23] = 0x00;
    data[24] = 0x05;
    data[25] = codec_data_size;
    memcpy(data + 26, codec_data, codec_data_size);
    data[26 + codec_data_size] = 0x06;
    data[26 + codec_data_size + 1] = 0x01;
    data[26 + codec_data_size + 2] = 0x02;
    md->setData(android::kKeyESDS, android::kTypeESDS, data + 4, codec_data_size + 29 - 4);
}

struct CodecDataConstructor {
    const char *mime;
    void (*cb)(android::sp<android::MetaData>, void *, ssize_t);
} CodecDataConstructors[] = {
    {android::MEDIA_MIMETYPE_VIDEO_MPEG4, mpeg4video},

    {NULL, NULL}
};

static void construct_codec_data(const char *mime, android::sp<android::MetaData> md,
                                 void *codec_data, ssize_t codec_data_size) {
    CodecDataConstructor *c = &CodecDataConstructors[0];
    while (c->mime != NULL) {
        if (!strcmp(c->mime, mime)) {
            c->cb(md, codec_data, codec_data_size);
            break;
        }
    }
}

class Buffers {
public:
    android::List<android::MediaBuffer *> buffers;
    android::Condition cond;
    android::Mutex lock;
};

class Source : public android::MediaSource {
    android::status_t start(android::MetaData *meta) {
        fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
        return android::OK;
    }

    android::sp<android::MetaData> getFormat() {
        // TODO: The only way to pass rotation is via a key here kKeyRotation
        fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
        return android::sp<android::MetaData>(new android::MetaData);
    }

    android::status_t stop() {
        fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
        return android::OK;
    }

    android::status_t read(android::MediaBuffer **buffer,
                           const android::MediaSource::ReadOptions *options = NULL) {
                fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
        return android::OK;
    }

    android::List<android::MediaBuffer *> m_framesReceived;
    android::List<android::MediaBuffer *> m_framesBeingEncoded;
};

class DroidMediaCodec
{
public:
    android::sp<android::MediaSource> m_codec;
    android::OMXClient *m_omx;
    android::sp<Source> m_src;
    Buffers m_waiting;
    Buffers m_processed;
//    android::sp<android::ALooper> m_looper;
};

extern "C" {

ssize_t droid_media_codec_find_by_type(const char *type, bool encoder)
{
    return android::MediaCodecList::getInstance()->findCodecByType(type, encoder);
}

ssize_t droid_media_codec_find_by_name(const char *name)
{
    return android::MediaCodecList::getInstance()->findCodecByName(name);
}

size_t droid_media_codec_count()
{
    return android::MediaCodecList::getInstance()->countCodecs();
}

const char *droid_media_codec_get_name(size_t index)
{
    return android::MediaCodecList::getInstance()->getCodecName(index);
}

bool droid_media_codec_is_encoder(size_t index)
{
    return android::MediaCodecList::getInstance()->isEncoder(index);
}

bool droid_media_codec_has_quirk(size_t index, const char *quirkName)
{
    return android::MediaCodecList::getInstance()->codecHasQuirk(index, quirkName);
}

char **droid_media_codec_get_supported_types(size_t index, ssize_t *size)
{
    android::Vector<android::AString> types;
    if (android::MediaCodecList::getInstance()->getSupportedTypes(index, &types) != android::OK) {
        return NULL;
    }

    char **out = (char **)malloc(types.size() + 1);
    for (size_t x = 0; x < types.size(); x++) {
        android::AString str = types[x];
        out[x] = (char *)malloc(str.size() + 1);
        memcpy(out[x], str.c_str(), str.size());
        out[x][str.size()] = '\0';
    }

    out[types.size()] = NULL;

    *size = types.size();

    return out;
}

bool droid_media_codec_get_capabilities(size_t index, const char *type,
                                        uint32_t **profiles, uint32_t **levels, ssize_t *profiles_levels_size,
                                        uint32_t **color_formats, ssize_t *color_formats_size)
{
    android::Vector<android::MediaCodecList::ProfileLevel> profileLevels;
    android::Vector<uint32_t> colorFormats;
    if (android::MediaCodecList::getInstance()->getCodecCapabilities(index, type,
                                                                     &profileLevels,
                                                                     &colorFormats) != android::OK) {
        ALOGE("DroidMediaCodec: Failed to get codec capabilities for %s", type);
        return false;
    }

    *profiles_levels_size = profileLevels.size();
    *profiles = (uint32_t *)malloc(*profiles_levels_size * sizeof(uint32_t));
    *levels = (uint32_t *)malloc(*profiles_levels_size * sizeof(uint32_t));

    for (ssize_t x = 0; x < *profiles_levels_size; x++) {
        (*profiles)[x] = profileLevels[x].mProfile;
        (*levels)[x] = profileLevels[x].mLevel;
    }

    *color_formats_size = colorFormats.size();
    *color_formats = (uint32_t *)malloc(*color_formats_size * sizeof(uint32_t));
    for (ssize_t x = 0; x < *color_formats_size; x++) {
        (*color_formats)[x] = colorFormats[x];
    }

    return true;
}

DroidMediaCodec *droid_media_codec_create(DroidMediaCodecMetaData *meta, DroidMediaCodecFlags flags)
{
    android::OMXClient *omx = new android::OMXClient;
    if (omx->connect() != android::OK) {
        ALOGE("DroidMediaCodec: Failed to connect to OMX");
        delete omx;
        return NULL;
    }

    // We will not do any validation for the flags. Stagefright should take care of that.
    uint32_t codec_flags = 0;
    if (flags & DROID_MEDIA_CODEC_SW_ONLY) {
        codec_flags |= android::OMXCodec::kSoftwareCodecsOnly;
    }

    if (flags & DROID_MEDIA_CODEC_HW_ONLY) {
        codec_flags |= android::OMXCodec::kHardwareCodecsOnly;
    }

    if (flags & DROID_MEDIA_CODEC_STORE_META_DATA_IN_VIDEO_BUFFERS) {
        codec_flags |= android::OMXCodec::kStoreMetaDataInVideoBuffers;
    }

    android::sp<Source> src(new Source);

    android::sp<android::MetaData> md(new android::MetaData);
    md->setCString(android::kKeyMIMEType, meta->type);
    md->setInt32(android::kKeyWidth, meta->width);
    md->setInt32(android::kKeyHeight, meta->height);
    md->setInt32(android::kKeyFrameRate, meta->fps);
    if (meta->codec_data_size > 0) {
        construct_codec_data (meta->type, md, meta->codec_data, meta->codec_data_size);
    }

    android::sp<android::MediaSource> codec
        = android::OMXCodec::Create(omx->interface(),
                                    md,
                                    flags & DROID_MEDIA_CODEC_ENCODER ? true : false,
                                    src,
                                    NULL, codec_flags, NULL);

    if (codec == NULL) {
        ALOGE("DroidMediaCodec: Failed to create codec");
        omx->disconnect();
        delete omx;
        return NULL;
    }

    DroidMediaCodec *mediaCodec = new DroidMediaCodec;
    mediaCodec->m_codec = codec;
    mediaCodec->m_omx = omx;
    mediaCodec->m_src = src;

/*
    mediaCodec->m_looper = new android::ALooper;
    int err = mediaCodec->m_looper->start();
    if (err != android::OK) {
        ALOGE("DroidMediaCodec: Error 0x%x starting looper", -err);
        delete omx;
        return NULL;
    }
*/
    return mediaCodec;
}

bool droid_media_codec_start(DroidMediaCodec *codec)
{
    int err = codec->m_codec->start();
    if (err != android::OK) {
        ALOGE("DroidMediaCodec: error 0x%x starting codec", -err);
        return false;
    }

    return true;
}

void droid_media_codec_stop(DroidMediaCodec *codec)
{
    int err = codec->m_codec->stop();
    if (err != android::OK) {
        ALOGE("DroidMediaCodec: error 0x%x stopping codec", -err);
    }
}

void droid_media_codec_destroy(DroidMediaCodec *codec)
{
//    int err;
// TODO:
    codec->m_omx->disconnect();
/*
    err = codec->m_looper->stop();
    if (err != android::OK) {
        ALOGE("DroidMediaCodec: Error 0x%x stopping looper", -err);
    }
*/
    delete codec->m_omx;
    delete codec;
}

void droid_media_codec_write(DroidMediaCodec *codec, DroidMediaData *data, DroidMediaBufferCallbacks *cb)
{

}

bool droid_media_codec_read(DroidMediaCodec *codec)
{
    int err;
    android::MediaBuffer *buffer = NULL;
    err = codec->m_codec->read(&buffer);

    if (err != android::OK) {
        ALOGE("DroidMediaCodec: Error 0x%x reading from codec", -err);
        return false;
    }

    return true;
}

};
