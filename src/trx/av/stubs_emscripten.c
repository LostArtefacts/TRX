// Emscripten stub implementations for FFmpeg-dependent subsystems.
//
// Audio_Sample_* and Audio_Stream_* are now provided by
// audio_sample_emscripten.c and audio_stream_emscripten.c respectively.
//
// Image_* functions are now provided by image_emscripten.c.
//
// This file is kept as a placeholder in case future FFmpeg-dependent
// subsystems need stubs on the web platform.

#ifdef EMSCRIPTEN_BUILD

// Currently empty — all previously stubbed functions now have real
// implementations in their respective *_emscripten.c files.

#endif // EMSCRIPTEN_BUILD
