#include <jni.h>

#include <string.h>
#include <zlib.h>

#include <new>

#define								SIZE							10

static z_stream*			ourStreams[SIZE]			= { 0 };

extern "C"
jint Java_org_amse_ys_zip_DeflatingDecompressor_startInflating(JNIEnv *env, jobject thiz) {
	int i;
	for (i = 0; i < SIZE; ++i) {
		if (ourStreams[i] == 0) {
			ourStreams[i] = new z_stream;
			memset(ourStreams[i], 0, sizeof(z_stream));
			inflateInit2(ourStreams[i], -MAX_WBITS);
			return i;
		}
	}
	return -1;
}

extern "C"
void Java_org_amse_ys_zip_DeflatingDecompressor_endInflating(JNIEnv *env, jobject thiz, jint inflatorId) {
	if (inflatorId >= 0 && inflatorId < SIZE) {
		inflateEnd(ourStreams[inflatorId]);
		delete ourStreams[inflatorId];
		ourStreams[inflatorId] = 0;
	}
}

// returns (endFlag << 32) + ((used inLength) << 16) + outLength
extern "C"
jlong Java_org_amse_ys_zip_DeflatingDecompressor_inflate(JNIEnv *env, jobject thiz, jint inflatorId, jbyteArray in, jint inOffset, jint inLength, jbyteArray out) {
	if (inflatorId < 0 || inflatorId >= SIZE) {
		return -1;
	}
	z_stream *stream = ourStreams[inflatorId];
	if (stream == 0) {
		return -2;
	}

	jbyte* inStart = env->GetByteArrayElements(in, 0);
	jbyte* outStart = env->GetByteArrayElements(out, 0);
	stream->next_in = (Bytef*)inStart + inOffset;
	stream->avail_in = inLength;
	stream->next_out = (Bytef*)outStart;
	const int outLength = env->GetArrayLength(out);
	stream->avail_out = outLength;
	const int code = inflate(stream, Z_SYNC_FLUSH);
	env->ReleaseByteArrayElements(in, inStart, 0);
	env->ReleaseByteArrayElements(out, outStart, 0);
	if (code == Z_OK || code == Z_STREAM_END) {
		jlong result = ((inLength - stream->avail_in) << 16) + outLength - stream->avail_out;
		if (code == Z_STREAM_END) {
			result |= ((jlong)1) << 32;
		}
		return result;
	}
	return -3;
}
