#include "common.h"
#include "sha256.h"

static SHA256_CTX global_ctx;
static uint8_t global_out[32];

static Nan::Persistent<v8::FunctionTemplate> sha256_constructor;

BSHA256::BSHA256() {
  memset(&ctx, 0, sizeof(SHA256_CTX));
}

BSHA256::~BSHA256() {}

void
BSHA256::Init(v8::Local<v8::Object> &target) {
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl =
    Nan::New<v8::FunctionTemplate>(BSHA256::New);

  sha256_constructor.Reset(tpl);

  tpl->SetClassName(Nan::New("SHA256").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "init", BSHA256::Init);
  Nan::SetPrototypeMethod(tpl, "update", BSHA256::Update);
  Nan::SetPrototypeMethod(tpl, "final", BSHA256::Final);
  Nan::SetMethod(tpl, "digest", BSHA256::Digest);
  Nan::SetMethod(tpl, "root", BSHA256::Root);
  Nan::SetMethod(tpl, "multi", BSHA256::Multi);

  v8::Local<v8::FunctionTemplate> ctor =
    Nan::New<v8::FunctionTemplate>(sha256_constructor);

  target->Set(Nan::New("SHA256").ToLocalChecked(), ctor->GetFunction());
}

NAN_METHOD(BSHA256::New) {
  if (!info.IsConstructCall())
    return Nan::ThrowError("Could not create SHA256 instance.");

  BSHA256 *sha = new BSHA256();
  sha->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(BSHA256::Init) {
  BSHA256 *sha = ObjectWrap::Unwrap<BSHA256>(info.Holder());

  SHA256_Init(&sha->ctx);

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(BSHA256::Update) {
  BSHA256 *sha = ObjectWrap::Unwrap<BSHA256>(info.Holder());

  if (info.Length() < 1)
    return Nan::ThrowError("sha256.update() requires arguments.");

  v8::Local<v8::Object> buf = info[0].As<v8::Object>();

  if (!node::Buffer::HasInstance(buf))
    return Nan::ThrowTypeError("First argument must be a buffer.");

  const uint8_t *in = (uint8_t *)node::Buffer::Data(buf);
  size_t inlen = node::Buffer::Length(buf);

  SHA256_Update(&sha->ctx, in, inlen);

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(BSHA256::Final) {
  BSHA256 *sha = ObjectWrap::Unwrap<BSHA256>(info.Holder());

  SHA256_Final(global_out, &sha->ctx);

  info.GetReturnValue().Set(
    Nan::CopyBuffer((char *)&global_out[0], 32).ToLocalChecked());
}

NAN_METHOD(BSHA256::Digest) {
  if (info.Length() < 1)
    return Nan::ThrowError("sha256.digest() requires arguments.");

  v8::Local<v8::Object> buf = info[0].As<v8::Object>();

  if (!node::Buffer::HasInstance(buf))
    return Nan::ThrowTypeError("First argument must be a buffer.");

  const uint8_t *in = (uint8_t *)node::Buffer::Data(buf);
  size_t inlen = node::Buffer::Length(buf);

  SHA256_Init(&global_ctx);
  SHA256_Update(&global_ctx, in, inlen);
  SHA256_Final(global_out, &global_ctx);

  info.GetReturnValue().Set(
    Nan::CopyBuffer((char *)&global_out[0], 32).ToLocalChecked());
}

NAN_METHOD(BSHA256::Root) {
  if (info.Length() < 2)
    return Nan::ThrowError("sha256.root() requires arguments.");

  v8::Local<v8::Object> lbuf = info[0].As<v8::Object>();
  v8::Local<v8::Object> rbuf = info[1].As<v8::Object>();

  if (!node::Buffer::HasInstance(lbuf))
    return Nan::ThrowTypeError("First argument must be a buffer.");

  if (!node::Buffer::HasInstance(rbuf))
    return Nan::ThrowTypeError("Second argument must be a buffer.");

  const uint8_t *left = (uint8_t *)node::Buffer::Data(lbuf);
  const uint8_t *right = (uint8_t *)node::Buffer::Data(rbuf);

  size_t leftlen = node::Buffer::Length(lbuf);
  size_t rightlen = node::Buffer::Length(rbuf);

  if (leftlen != 32 || rightlen != 32)
    return Nan::ThrowTypeError("Bad node sizes.");

  SHA256_Init(&global_ctx);
  SHA256_Update(&global_ctx, left, leftlen);
  SHA256_Update(&global_ctx, right, rightlen);
  SHA256_Final(global_out, &global_ctx);

  info.GetReturnValue().Set(
    Nan::CopyBuffer((char *)&global_out[0], 32).ToLocalChecked());
}

NAN_METHOD(BSHA256::Multi) {
  if (info.Length() < 2)
    return Nan::ThrowError("sha256.multi() requires arguments.");

  v8::Local<v8::Object> xbuf = info[0].As<v8::Object>();
  v8::Local<v8::Object> ybuf = info[1].As<v8::Object>();

  if (!node::Buffer::HasInstance(xbuf))
    return Nan::ThrowTypeError("First argument must be a buffer.");

  if (!node::Buffer::HasInstance(ybuf))
    return Nan::ThrowTypeError("Second argument must be a buffer.");

  const uint8_t *x = (uint8_t *)node::Buffer::Data(xbuf);
  const uint8_t *y = (uint8_t *)node::Buffer::Data(ybuf);

  size_t xlen = node::Buffer::Length(xbuf);
  size_t ylen = node::Buffer::Length(ybuf);

  const uint8_t *z = NULL;
  size_t zlen = 0;

  if (info.Length() > 2 && !IsNull(info[2])) {
    v8::Local<v8::Object> zbuf = info[2].As<v8::Object>();

    if (!node::Buffer::HasInstance(zbuf))
      return Nan::ThrowTypeError("Third argument must be a buffer.");

    z = (const uint8_t *)node::Buffer::Data(zbuf);
    zlen = node::Buffer::Length(zbuf);
  }

  SHA256_Init(&global_ctx);
  SHA256_Update(&global_ctx, x, xlen);
  SHA256_Update(&global_ctx, y, ylen);
  if (z)
    SHA256_Update(&global_ctx, z, zlen);
  SHA256_Final(global_out, &global_ctx);

  info.GetReturnValue().Set(
    Nan::CopyBuffer((char *)&global_out[0], 32).ToLocalChecked());
}
