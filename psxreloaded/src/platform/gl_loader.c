#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gl_loader.h"

static void* get_proc(const char *namez);

#if defined(_WIN32) || defined(__CYGWIN__)
#ifndef _WINDOWS_
#undef APIENTRY
#endif
#include <windows.h>
static HMODULE libGL;

typedef void* (APIENTRYP PFNWGLGETPROCADDRESSPROC_PRIVATE)(const char*);
static PFNWGLGETPROCADDRESSPROC_PRIVATE gl_loaderGetProcAddressPtr;

#ifdef _MSC_VER
#ifdef __has_include
  #if __has_include(<winapifamily.h>)
    #define HAVE_WINAPIFAMILY 1
  #endif
#elif _MSC_VER >= 1700 && !_USING_V110_SDK71_
  #define HAVE_WINAPIFAMILY 1
#endif
#endif

#ifdef HAVE_WINAPIFAMILY
  #include <winapifamily.h>
  #if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
    #define IS_UWP 1
  #endif
#endif

static
int open_gl(void) {
#ifndef IS_UWP
    libGL = LoadLibraryW(L"opengl32.dll");
    if(libGL != NULL) {
        void (* tmp)(void);
        tmp = (void(*)(void)) GetProcAddress(libGL, "wglGetProcAddress");
        gl_loaderGetProcAddressPtr = (PFNWGLGETPROCADDRESSPROC_PRIVATE) tmp;
        return gl_loaderGetProcAddressPtr != NULL;
    }
#endif

    return 0;
}

static
void close_gl(void) {
    if(libGL != NULL) {
        FreeLibrary((HMODULE) libGL);
        libGL = NULL;
    }
}
#else
#include <dlfcn.h>
static void* libGL;

#if !defined(__APPLE__) && !defined(__HAIKU__)
typedef void* (APIENTRYP PFNGLXGETPROCADDRESSPROC_PRIVATE)(const char*);
static PFNGLXGETPROCADDRESSPROC_PRIVATE gl_loaderGetProcAddressPtr;
#endif

static
int open_gl(void) {
#ifdef __APPLE__
    static const char *NAMES[] = {
        "../Frameworks/OpenGL.framework/OpenGL",
        "/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"
    };
#else
    static const char *NAMES[] = {"libGL.so.1", "libGL.so"};
#endif

    unsigned int index = 0;
    for(index = 0; index < (sizeof(NAMES) / sizeof(NAMES[0])); index++) {
        libGL = dlopen(NAMES[index], RTLD_NOW | RTLD_GLOBAL);

        if(libGL != NULL) {
#if defined(__APPLE__) || defined(__HAIKU__)
            return 1;
#else
            gl_loaderGetProcAddressPtr = (PFNGLXGETPROCADDRESSPROC_PRIVATE)dlsym(libGL,
                "glXGetProcAddressARB");
            return gl_loaderGetProcAddressPtr != NULL;
#endif
        }
    }

    return 0;
}

static
void close_gl(void) {
    if(libGL != NULL) {
        dlclose(libGL);
        libGL = NULL;
    }
}
#endif

static
void* get_proc(const char *namez) {
    void* result = NULL;
    if(libGL == NULL) return NULL;

#if !defined(__APPLE__) && !defined(__HAIKU__)
    if(gl_loaderGetProcAddressPtr != NULL) {
        result = gl_loaderGetProcAddressPtr(namez);
    }
#endif
    if(result == NULL) {
#if defined(_WIN32) || defined(__CYGWIN__)
        result = (void*)GetProcAddress((HMODULE) libGL, namez);
#else
        result = dlsym(libGL, namez);
#endif
    }

    return result;
}

int gl_loaderLoadGL(void) {
    int status = 0;

    if(open_gl()) {
        status = gl_loaderLoadGLLoader(&get_proc);
        close_gl();
    }

    return status;
}

struct gl_loaderGLversionStruct GLVersion = { 0, 0 };

#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_3_0)
#define _GLLOADER_IS_SOME_NEW_VERSION 1
#endif

static int max_loaded_major;
static int max_loaded_minor;

static const char *exts = NULL;
static int num_exts_i = 0;
static char **exts_i = NULL;

static int get_exts(void) {
#ifdef _GLLOADER_IS_SOME_NEW_VERSION
    if(max_loaded_major < 3) {
#endif
        exts = (const char *)glGetString(GL_EXTENSIONS);
#ifdef _GLLOADER_IS_SOME_NEW_VERSION
    } else {
        unsigned int index;

        num_exts_i = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &num_exts_i);
        if (num_exts_i > 0) {
            exts_i = (char **)malloc((size_t)num_exts_i * (sizeof *exts_i));
        }

        if (exts_i == NULL) {
            return 0;
        }

        for(index = 0; index < (unsigned)num_exts_i; index++) {
            const char *gl_str_tmp = (const char*)glGetStringi(GL_EXTENSIONS, index);
            size_t len = strlen(gl_str_tmp);

            char *local_str = (char*)malloc((len+1) * sizeof(char));
            if(local_str != NULL) {
                memcpy(local_str, gl_str_tmp, (len+1) * sizeof(char));
            }
            exts_i[index] = local_str;
        }
    }
#endif
    return 1;
}

static void free_exts(void) {
    if (exts_i != NULL) {
        int index;
        for(index = 0; index < num_exts_i; index++) {
            free((char *)exts_i[index]);
        }
        free((void *)exts_i);
        exts_i = NULL;
    }
}

static int has_ext(const char *ext) {
#ifdef _GLLOADER_IS_SOME_NEW_VERSION
    if(max_loaded_major < 3) {
#endif
        const char *extensions;
        const char *loc;
        const char *terminator;
        extensions = exts;
        if(extensions == NULL || ext == NULL) {
            return 0;
        }

        while(1) {
            loc = strstr(extensions, ext);
            if(loc == NULL) {
                return 0;
            }

            terminator = loc + strlen(ext);
            if((loc == extensions || *(loc - 1) == ' ') &&
                (*terminator == ' ' || *terminator == '\0')) {
                return 1;
            }
            extensions = terminator;
        }
#ifdef _GLLOADER_IS_SOME_NEW_VERSION
    } else {
        int index;
        if(exts_i == NULL) return 0;
        for(index = 0; index < num_exts_i; index++) {
            const char *e = exts_i[index];

            if(exts_i[index] != NULL && strcmp(e, ext) == 0) {
                return 1;
            }
        }
    }
#endif

    return 0;
}
int GLLOADER_GL_VERSION_1_0 = 0;
int GLLOADER_GL_VERSION_1_1 = 0;
int GLLOADER_GL_VERSION_1_2 = 0;
int GLLOADER_GL_VERSION_1_3 = 0;
int GLLOADER_GL_VERSION_1_4 = 0;
int GLLOADER_GL_VERSION_1_5 = 0;
int GLLOADER_GL_VERSION_2_0 = 0;
int GLLOADER_GL_VERSION_2_1 = 0;
int GLLOADER_GL_VERSION_3_0 = 0;
int GLLOADER_GL_VERSION_3_1 = 0;
int GLLOADER_GL_VERSION_3_2 = 0;
int GLLOADER_GL_VERSION_3_3 = 0;
int GLLOADER_GL_VERSION_4_0 = 0;
int GLLOADER_GL_VERSION_4_1 = 0;
int GLLOADER_GL_VERSION_4_2 = 0;
int GLLOADER_GL_VERSION_4_3 = 0;
int GLLOADER_GL_VERSION_4_4 = 0;
int GLLOADER_GL_VERSION_4_5 = 0;
PFNGLACTIVESHADERPROGRAMPROC gl_loader_glActiveShaderProgram = NULL;
PFNGLACTIVETEXTUREPROC gl_loader_glActiveTexture = NULL;
PFNGLATTACHSHADERPROC gl_loader_glAttachShader = NULL;
PFNGLBEGINCONDITIONALRENDERPROC gl_loader_glBeginConditionalRender = NULL;
PFNGLBEGINQUERYPROC gl_loader_glBeginQuery = NULL;
PFNGLBEGINQUERYINDEXEDPROC gl_loader_glBeginQueryIndexed = NULL;
PFNGLBEGINTRANSFORMFEEDBACKPROC gl_loader_glBeginTransformFeedback = NULL;
PFNGLBINDATTRIBLOCATIONPROC gl_loader_glBindAttribLocation = NULL;
PFNGLBINDBUFFERPROC gl_loader_glBindBuffer = NULL;
PFNGLBINDBUFFERBASEPROC gl_loader_glBindBufferBase = NULL;
PFNGLBINDBUFFERRANGEPROC gl_loader_glBindBufferRange = NULL;
PFNGLBINDBUFFERSBASEPROC gl_loader_glBindBuffersBase = NULL;
PFNGLBINDBUFFERSRANGEPROC gl_loader_glBindBuffersRange = NULL;
PFNGLBINDFRAGDATALOCATIONPROC gl_loader_glBindFragDataLocation = NULL;
PFNGLBINDFRAGDATALOCATIONINDEXEDPROC gl_loader_glBindFragDataLocationIndexed = NULL;
PFNGLBINDFRAMEBUFFERPROC gl_loader_glBindFramebuffer = NULL;
PFNGLBINDIMAGETEXTUREPROC gl_loader_glBindImageTexture = NULL;
PFNGLBINDIMAGETEXTURESPROC gl_loader_glBindImageTextures = NULL;
PFNGLBINDPROGRAMPIPELINEPROC gl_loader_glBindProgramPipeline = NULL;
PFNGLBINDRENDERBUFFERPROC gl_loader_glBindRenderbuffer = NULL;
PFNGLBINDSAMPLERPROC gl_loader_glBindSampler = NULL;
PFNGLBINDSAMPLERSPROC gl_loader_glBindSamplers = NULL;
PFNGLBINDTEXTUREPROC gl_loader_glBindTexture = NULL;
PFNGLBINDTEXTUREUNITPROC gl_loader_glBindTextureUnit = NULL;
PFNGLBINDTEXTURESPROC gl_loader_glBindTextures = NULL;
PFNGLBINDTRANSFORMFEEDBACKPROC gl_loader_glBindTransformFeedback = NULL;
PFNGLBINDVERTEXARRAYPROC gl_loader_glBindVertexArray = NULL;
PFNGLBINDVERTEXBUFFERPROC gl_loader_glBindVertexBuffer = NULL;
PFNGLBINDVERTEXBUFFERSPROC gl_loader_glBindVertexBuffers = NULL;
PFNGLBLENDCOLORPROC gl_loader_glBlendColor = NULL;
PFNGLBLENDEQUATIONPROC gl_loader_glBlendEquation = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC gl_loader_glBlendEquationSeparate = NULL;
PFNGLBLENDEQUATIONSEPARATEIPROC gl_loader_glBlendEquationSeparatei = NULL;
PFNGLBLENDEQUATIONIPROC gl_loader_glBlendEquationi = NULL;
PFNGLBLENDFUNCPROC gl_loader_glBlendFunc = NULL;
PFNGLBLENDFUNCSEPARATEPROC gl_loader_glBlendFuncSeparate = NULL;
PFNGLBLENDFUNCSEPARATEIPROC gl_loader_glBlendFuncSeparatei = NULL;
PFNGLBLENDFUNCIPROC gl_loader_glBlendFunci = NULL;
PFNGLBLITFRAMEBUFFERPROC gl_loader_glBlitFramebuffer = NULL;
PFNGLBLITNAMEDFRAMEBUFFERPROC gl_loader_glBlitNamedFramebuffer = NULL;
PFNGLBUFFERDATAPROC gl_loader_glBufferData = NULL;
PFNGLBUFFERSTORAGEPROC gl_loader_glBufferStorage = NULL;
PFNGLBUFFERSUBDATAPROC gl_loader_glBufferSubData = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC gl_loader_glCheckFramebufferStatus = NULL;
PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC gl_loader_glCheckNamedFramebufferStatus = NULL;
PFNGLCLAMPCOLORPROC gl_loader_glClampColor = NULL;
PFNGLCLEARPROC gl_loader_glClear = NULL;
PFNGLCLEARBUFFERDATAPROC gl_loader_glClearBufferData = NULL;
PFNGLCLEARBUFFERSUBDATAPROC gl_loader_glClearBufferSubData = NULL;
PFNGLCLEARBUFFERFIPROC gl_loader_glClearBufferfi = NULL;
PFNGLCLEARBUFFERFVPROC gl_loader_glClearBufferfv = NULL;
PFNGLCLEARBUFFERIVPROC gl_loader_glClearBufferiv = NULL;
PFNGLCLEARBUFFERUIVPROC gl_loader_glClearBufferuiv = NULL;
PFNGLCLEARCOLORPROC gl_loader_glClearColor = NULL;
PFNGLCLEARDEPTHPROC gl_loader_glClearDepth = NULL;
PFNGLCLEARDEPTHFPROC gl_loader_glClearDepthf = NULL;
PFNGLCLEARNAMEDBUFFERDATAPROC gl_loader_glClearNamedBufferData = NULL;
PFNGLCLEARNAMEDBUFFERSUBDATAPROC gl_loader_glClearNamedBufferSubData = NULL;
PFNGLCLEARNAMEDFRAMEBUFFERFIPROC gl_loader_glClearNamedFramebufferfi = NULL;
PFNGLCLEARNAMEDFRAMEBUFFERFVPROC gl_loader_glClearNamedFramebufferfv = NULL;
PFNGLCLEARNAMEDFRAMEBUFFERIVPROC gl_loader_glClearNamedFramebufferiv = NULL;
PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC gl_loader_glClearNamedFramebufferuiv = NULL;
PFNGLCLEARSTENCILPROC gl_loader_glClearStencil = NULL;
PFNGLCLEARTEXIMAGEPROC gl_loader_glClearTexImage = NULL;
PFNGLCLEARTEXSUBIMAGEPROC gl_loader_glClearTexSubImage = NULL;
PFNGLCLIENTWAITSYNCPROC gl_loader_glClientWaitSync = NULL;
PFNGLCLIPCONTROLPROC gl_loader_glClipControl = NULL;
PFNGLCOLORMASKPROC gl_loader_glColorMask = NULL;
PFNGLCOLORMASKIPROC gl_loader_glColorMaski = NULL;
PFNGLCOLORP3UIPROC gl_loader_glColorP3ui = NULL;
PFNGLCOLORP3UIVPROC gl_loader_glColorP3uiv = NULL;
PFNGLCOLORP4UIPROC gl_loader_glColorP4ui = NULL;
PFNGLCOLORP4UIVPROC gl_loader_glColorP4uiv = NULL;
PFNGLCOMPILESHADERPROC gl_loader_glCompileShader = NULL;
PFNGLCOMPRESSEDTEXIMAGE1DPROC gl_loader_glCompressedTexImage1D = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC gl_loader_glCompressedTexImage2D = NULL;
PFNGLCOMPRESSEDTEXIMAGE3DPROC gl_loader_glCompressedTexImage3D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC gl_loader_glCompressedTexSubImage1D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC gl_loader_glCompressedTexSubImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC gl_loader_glCompressedTexSubImage3D = NULL;
PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC gl_loader_glCompressedTextureSubImage1D = NULL;
PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC gl_loader_glCompressedTextureSubImage2D = NULL;
PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC gl_loader_glCompressedTextureSubImage3D = NULL;
PFNGLCOPYBUFFERSUBDATAPROC gl_loader_glCopyBufferSubData = NULL;
PFNGLCOPYIMAGESUBDATAPROC gl_loader_glCopyImageSubData = NULL;
PFNGLCOPYNAMEDBUFFERSUBDATAPROC gl_loader_glCopyNamedBufferSubData = NULL;
PFNGLCOPYTEXIMAGE1DPROC gl_loader_glCopyTexImage1D = NULL;
PFNGLCOPYTEXIMAGE2DPROC gl_loader_glCopyTexImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE1DPROC gl_loader_glCopyTexSubImage1D = NULL;
PFNGLCOPYTEXSUBIMAGE2DPROC gl_loader_glCopyTexSubImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE3DPROC gl_loader_glCopyTexSubImage3D = NULL;
PFNGLCOPYTEXTURESUBIMAGE1DPROC gl_loader_glCopyTextureSubImage1D = NULL;
PFNGLCOPYTEXTURESUBIMAGE2DPROC gl_loader_glCopyTextureSubImage2D = NULL;
PFNGLCOPYTEXTURESUBIMAGE3DPROC gl_loader_glCopyTextureSubImage3D = NULL;
PFNGLCREATEBUFFERSPROC gl_loader_glCreateBuffers = NULL;
PFNGLCREATEFRAMEBUFFERSPROC gl_loader_glCreateFramebuffers = NULL;
PFNGLCREATEPROGRAMPROC gl_loader_glCreateProgram = NULL;
PFNGLCREATEPROGRAMPIPELINESPROC gl_loader_glCreateProgramPipelines = NULL;
PFNGLCREATEQUERIESPROC gl_loader_glCreateQueries = NULL;
PFNGLCREATERENDERBUFFERSPROC gl_loader_glCreateRenderbuffers = NULL;
PFNGLCREATESAMPLERSPROC gl_loader_glCreateSamplers = NULL;
PFNGLCREATESHADERPROC gl_loader_glCreateShader = NULL;
PFNGLCREATESHADERPROGRAMVPROC gl_loader_glCreateShaderProgramv = NULL;
PFNGLCREATETEXTURESPROC gl_loader_glCreateTextures = NULL;
PFNGLCREATETRANSFORMFEEDBACKSPROC gl_loader_glCreateTransformFeedbacks = NULL;
PFNGLCREATEVERTEXARRAYSPROC gl_loader_glCreateVertexArrays = NULL;
PFNGLCULLFACEPROC gl_loader_glCullFace = NULL;
PFNGLDEBUGMESSAGECALLBACKPROC gl_loader_glDebugMessageCallback = NULL;
PFNGLDEBUGMESSAGECONTROLPROC gl_loader_glDebugMessageControl = NULL;
PFNGLDEBUGMESSAGEINSERTPROC gl_loader_glDebugMessageInsert = NULL;
PFNGLDELETEBUFFERSPROC gl_loader_glDeleteBuffers = NULL;
PFNGLDELETEFRAMEBUFFERSPROC gl_loader_glDeleteFramebuffers = NULL;
PFNGLDELETEPROGRAMPROC gl_loader_glDeleteProgram = NULL;
PFNGLDELETEPROGRAMPIPELINESPROC gl_loader_glDeleteProgramPipelines = NULL;
PFNGLDELETEQUERIESPROC gl_loader_glDeleteQueries = NULL;
PFNGLDELETERENDERBUFFERSPROC gl_loader_glDeleteRenderbuffers = NULL;
PFNGLDELETESAMPLERSPROC gl_loader_glDeleteSamplers = NULL;
PFNGLDELETESHADERPROC gl_loader_glDeleteShader = NULL;
PFNGLDELETESYNCPROC gl_loader_glDeleteSync = NULL;
PFNGLDELETETEXTURESPROC gl_loader_glDeleteTextures = NULL;
PFNGLDELETETRANSFORMFEEDBACKSPROC gl_loader_glDeleteTransformFeedbacks = NULL;
PFNGLDELETEVERTEXARRAYSPROC gl_loader_glDeleteVertexArrays = NULL;
PFNGLDEPTHFUNCPROC gl_loader_glDepthFunc = NULL;
PFNGLDEPTHMASKPROC gl_loader_glDepthMask = NULL;
PFNGLDEPTHRANGEPROC gl_loader_glDepthRange = NULL;
PFNGLDEPTHRANGEARRAYVPROC gl_loader_glDepthRangeArrayv = NULL;
PFNGLDEPTHRANGEINDEXEDPROC gl_loader_glDepthRangeIndexed = NULL;
PFNGLDEPTHRANGEFPROC gl_loader_glDepthRangef = NULL;
PFNGLDETACHSHADERPROC gl_loader_glDetachShader = NULL;
PFNGLDISABLEPROC gl_loader_glDisable = NULL;
PFNGLDISABLEVERTEXARRAYATTRIBPROC gl_loader_glDisableVertexArrayAttrib = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC gl_loader_glDisableVertexAttribArray = NULL;
PFNGLDISABLEIPROC gl_loader_glDisablei = NULL;
PFNGLDISPATCHCOMPUTEPROC gl_loader_glDispatchCompute = NULL;
PFNGLDISPATCHCOMPUTEINDIRECTPROC gl_loader_glDispatchComputeIndirect = NULL;
PFNGLDRAWARRAYSPROC gl_loader_glDrawArrays = NULL;
PFNGLDRAWARRAYSINDIRECTPROC gl_loader_glDrawArraysIndirect = NULL;
PFNGLDRAWARRAYSINSTANCEDPROC gl_loader_glDrawArraysInstanced = NULL;
PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC gl_loader_glDrawArraysInstancedBaseInstance = NULL;
PFNGLDRAWBUFFERPROC gl_loader_glDrawBuffer = NULL;
PFNGLDRAWBUFFERSPROC gl_loader_glDrawBuffers = NULL;
PFNGLDRAWELEMENTSPROC gl_loader_glDrawElements = NULL;
PFNGLDRAWELEMENTSBASEVERTEXPROC gl_loader_glDrawElementsBaseVertex = NULL;
PFNGLDRAWELEMENTSINDIRECTPROC gl_loader_glDrawElementsIndirect = NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC gl_loader_glDrawElementsInstanced = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC gl_loader_glDrawElementsInstancedBaseInstance = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC gl_loader_glDrawElementsInstancedBaseVertex = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC gl_loader_glDrawElementsInstancedBaseVertexBaseInstance = NULL;
PFNGLDRAWRANGEELEMENTSPROC gl_loader_glDrawRangeElements = NULL;
PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC gl_loader_glDrawRangeElementsBaseVertex = NULL;
PFNGLDRAWTRANSFORMFEEDBACKPROC gl_loader_glDrawTransformFeedback = NULL;
PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC gl_loader_glDrawTransformFeedbackInstanced = NULL;
PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC gl_loader_glDrawTransformFeedbackStream = NULL;
PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC gl_loader_glDrawTransformFeedbackStreamInstanced = NULL;
PFNGLENABLEPROC gl_loader_glEnable = NULL;
PFNGLENABLEVERTEXARRAYATTRIBPROC gl_loader_glEnableVertexArrayAttrib = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC gl_loader_glEnableVertexAttribArray = NULL;
PFNGLENABLEIPROC gl_loader_glEnablei = NULL;
PFNGLENDCONDITIONALRENDERPROC gl_loader_glEndConditionalRender = NULL;
PFNGLENDQUERYPROC gl_loader_glEndQuery = NULL;
PFNGLENDQUERYINDEXEDPROC gl_loader_glEndQueryIndexed = NULL;
PFNGLENDTRANSFORMFEEDBACKPROC gl_loader_glEndTransformFeedback = NULL;
PFNGLFENCESYNCPROC gl_loader_glFenceSync = NULL;
PFNGLFINISHPROC gl_loader_glFinish = NULL;
PFNGLFLUSHPROC gl_loader_glFlush = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC gl_loader_glFlushMappedBufferRange = NULL;
PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC gl_loader_glFlushMappedNamedBufferRange = NULL;
PFNGLFRAMEBUFFERPARAMETERIPROC gl_loader_glFramebufferParameteri = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC gl_loader_glFramebufferRenderbuffer = NULL;
PFNGLFRAMEBUFFERTEXTUREPROC gl_loader_glFramebufferTexture = NULL;
PFNGLFRAMEBUFFERTEXTURE1DPROC gl_loader_glFramebufferTexture1D = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC gl_loader_glFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERTEXTURE3DPROC gl_loader_glFramebufferTexture3D = NULL;
PFNGLFRAMEBUFFERTEXTURELAYERPROC gl_loader_glFramebufferTextureLayer = NULL;
PFNGLFRONTFACEPROC gl_loader_glFrontFace = NULL;
PFNGLGENBUFFERSPROC gl_loader_glGenBuffers = NULL;
PFNGLGENFRAMEBUFFERSPROC gl_loader_glGenFramebuffers = NULL;
PFNGLGENPROGRAMPIPELINESPROC gl_loader_glGenProgramPipelines = NULL;
PFNGLGENQUERIESPROC gl_loader_glGenQueries = NULL;
PFNGLGENRENDERBUFFERSPROC gl_loader_glGenRenderbuffers = NULL;
PFNGLGENSAMPLERSPROC gl_loader_glGenSamplers = NULL;
PFNGLGENTEXTURESPROC gl_loader_glGenTextures = NULL;
PFNGLGENTRANSFORMFEEDBACKSPROC gl_loader_glGenTransformFeedbacks = NULL;
PFNGLGENVERTEXARRAYSPROC gl_loader_glGenVertexArrays = NULL;
PFNGLGENERATEMIPMAPPROC gl_loader_glGenerateMipmap = NULL;
PFNGLGENERATETEXTUREMIPMAPPROC gl_loader_glGenerateTextureMipmap = NULL;
PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC gl_loader_glGetActiveAtomicCounterBufferiv = NULL;
PFNGLGETACTIVEATTRIBPROC gl_loader_glGetActiveAttrib = NULL;
PFNGLGETACTIVESUBROUTINENAMEPROC gl_loader_glGetActiveSubroutineName = NULL;
PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC gl_loader_glGetActiveSubroutineUniformName = NULL;
PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC gl_loader_glGetActiveSubroutineUniformiv = NULL;
PFNGLGETACTIVEUNIFORMPROC gl_loader_glGetActiveUniform = NULL;
PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC gl_loader_glGetActiveUniformBlockName = NULL;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC gl_loader_glGetActiveUniformBlockiv = NULL;
PFNGLGETACTIVEUNIFORMNAMEPROC gl_loader_glGetActiveUniformName = NULL;
PFNGLGETACTIVEUNIFORMSIVPROC gl_loader_glGetActiveUniformsiv = NULL;
PFNGLGETATTACHEDSHADERSPROC gl_loader_glGetAttachedShaders = NULL;
PFNGLGETATTRIBLOCATIONPROC gl_loader_glGetAttribLocation = NULL;
PFNGLGETBOOLEANI_VPROC gl_loader_glGetBooleani_v = NULL;
PFNGLGETBOOLEANVPROC gl_loader_glGetBooleanv = NULL;
PFNGLGETBUFFERPARAMETERI64VPROC gl_loader_glGetBufferParameteri64v = NULL;
PFNGLGETBUFFERPARAMETERIVPROC gl_loader_glGetBufferParameteriv = NULL;
PFNGLGETBUFFERPOINTERVPROC gl_loader_glGetBufferPointerv = NULL;
PFNGLGETBUFFERSUBDATAPROC gl_loader_glGetBufferSubData = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEPROC gl_loader_glGetCompressedTexImage = NULL;
PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC gl_loader_glGetCompressedTextureImage = NULL;
PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC gl_loader_glGetCompressedTextureSubImage = NULL;
PFNGLGETDEBUGMESSAGELOGPROC gl_loader_glGetDebugMessageLog = NULL;
PFNGLGETDOUBLEI_VPROC gl_loader_glGetDoublei_v = NULL;
PFNGLGETDOUBLEVPROC gl_loader_glGetDoublev = NULL;
PFNGLGETERRORPROC gl_loader_glGetError = NULL;
PFNGLGETFLOATI_VPROC gl_loader_glGetFloati_v = NULL;
PFNGLGETFLOATVPROC gl_loader_glGetFloatv = NULL;
PFNGLGETFRAGDATAINDEXPROC gl_loader_glGetFragDataIndex = NULL;
PFNGLGETFRAGDATALOCATIONPROC gl_loader_glGetFragDataLocation = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC gl_loader_glGetFramebufferAttachmentParameteriv = NULL;
PFNGLGETFRAMEBUFFERPARAMETERIVPROC gl_loader_glGetFramebufferParameteriv = NULL;
PFNGLGETGRAPHICSRESETSTATUSPROC gl_loader_glGetGraphicsResetStatus = NULL;
PFNGLGETINTEGER64I_VPROC gl_loader_glGetInteger64i_v = NULL;
PFNGLGETINTEGER64VPROC gl_loader_glGetInteger64v = NULL;
PFNGLGETINTEGERI_VPROC gl_loader_glGetIntegeri_v = NULL;
PFNGLGETINTEGERVPROC gl_loader_glGetIntegerv = NULL;
PFNGLGETINTERNALFORMATI64VPROC gl_loader_glGetInternalformati64v = NULL;
PFNGLGETINTERNALFORMATIVPROC gl_loader_glGetInternalformativ = NULL;
PFNGLGETMULTISAMPLEFVPROC gl_loader_glGetMultisamplefv = NULL;
PFNGLGETNAMEDBUFFERPARAMETERI64VPROC gl_loader_glGetNamedBufferParameteri64v = NULL;
PFNGLGETNAMEDBUFFERPARAMETERIVPROC gl_loader_glGetNamedBufferParameteriv = NULL;
PFNGLGETNAMEDBUFFERPOINTERVPROC gl_loader_glGetNamedBufferPointerv = NULL;
PFNGLGETNAMEDBUFFERSUBDATAPROC gl_loader_glGetNamedBufferSubData = NULL;
PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC gl_loader_glGetNamedFramebufferAttachmentParameteriv = NULL;
PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC gl_loader_glGetNamedFramebufferParameteriv = NULL;
PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC gl_loader_glGetNamedRenderbufferParameteriv = NULL;
PFNGLGETOBJECTLABELPROC gl_loader_glGetObjectLabel = NULL;
PFNGLGETOBJECTPTRLABELPROC gl_loader_glGetObjectPtrLabel = NULL;
PFNGLGETPOINTERVPROC gl_loader_glGetPointerv = NULL;
PFNGLGETPROGRAMBINARYPROC gl_loader_glGetProgramBinary = NULL;
PFNGLGETPROGRAMINFOLOGPROC gl_loader_glGetProgramInfoLog = NULL;
PFNGLGETPROGRAMINTERFACEIVPROC gl_loader_glGetProgramInterfaceiv = NULL;
PFNGLGETPROGRAMPIPELINEINFOLOGPROC gl_loader_glGetProgramPipelineInfoLog = NULL;
PFNGLGETPROGRAMPIPELINEIVPROC gl_loader_glGetProgramPipelineiv = NULL;
PFNGLGETPROGRAMRESOURCEINDEXPROC gl_loader_glGetProgramResourceIndex = NULL;
PFNGLGETPROGRAMRESOURCELOCATIONPROC gl_loader_glGetProgramResourceLocation = NULL;
PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC gl_loader_glGetProgramResourceLocationIndex = NULL;
PFNGLGETPROGRAMRESOURCENAMEPROC gl_loader_glGetProgramResourceName = NULL;
PFNGLGETPROGRAMRESOURCEIVPROC gl_loader_glGetProgramResourceiv = NULL;
PFNGLGETPROGRAMSTAGEIVPROC gl_loader_glGetProgramStageiv = NULL;
PFNGLGETPROGRAMIVPROC gl_loader_glGetProgramiv = NULL;
PFNGLGETQUERYBUFFEROBJECTI64VPROC gl_loader_glGetQueryBufferObjecti64v = NULL;
PFNGLGETQUERYBUFFEROBJECTIVPROC gl_loader_glGetQueryBufferObjectiv = NULL;
PFNGLGETQUERYBUFFEROBJECTUI64VPROC gl_loader_glGetQueryBufferObjectui64v = NULL;
PFNGLGETQUERYBUFFEROBJECTUIVPROC gl_loader_glGetQueryBufferObjectuiv = NULL;
PFNGLGETQUERYINDEXEDIVPROC gl_loader_glGetQueryIndexediv = NULL;
PFNGLGETQUERYOBJECTI64VPROC gl_loader_glGetQueryObjecti64v = NULL;
PFNGLGETQUERYOBJECTIVPROC gl_loader_glGetQueryObjectiv = NULL;
PFNGLGETQUERYOBJECTUI64VPROC gl_loader_glGetQueryObjectui64v = NULL;
PFNGLGETQUERYOBJECTUIVPROC gl_loader_glGetQueryObjectuiv = NULL;
PFNGLGETQUERYIVPROC gl_loader_glGetQueryiv = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC gl_loader_glGetRenderbufferParameteriv = NULL;
PFNGLGETSAMPLERPARAMETERIIVPROC gl_loader_glGetSamplerParameterIiv = NULL;
PFNGLGETSAMPLERPARAMETERIUIVPROC gl_loader_glGetSamplerParameterIuiv = NULL;
PFNGLGETSAMPLERPARAMETERFVPROC gl_loader_glGetSamplerParameterfv = NULL;
PFNGLGETSAMPLERPARAMETERIVPROC gl_loader_glGetSamplerParameteriv = NULL;
PFNGLGETSHADERINFOLOGPROC gl_loader_glGetShaderInfoLog = NULL;
PFNGLGETSHADERPRECISIONFORMATPROC gl_loader_glGetShaderPrecisionFormat = NULL;
PFNGLGETSHADERSOURCEPROC gl_loader_glGetShaderSource = NULL;
PFNGLGETSHADERIVPROC gl_loader_glGetShaderiv = NULL;
PFNGLGETSTRINGPROC gl_loader_glGetString = NULL;
PFNGLGETSTRINGIPROC gl_loader_glGetStringi = NULL;
PFNGLGETSUBROUTINEINDEXPROC gl_loader_glGetSubroutineIndex = NULL;
PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC gl_loader_glGetSubroutineUniformLocation = NULL;
PFNGLGETSYNCIVPROC gl_loader_glGetSynciv = NULL;
PFNGLGETTEXIMAGEPROC gl_loader_glGetTexImage = NULL;
PFNGLGETTEXLEVELPARAMETERFVPROC gl_loader_glGetTexLevelParameterfv = NULL;
PFNGLGETTEXLEVELPARAMETERIVPROC gl_loader_glGetTexLevelParameteriv = NULL;
PFNGLGETTEXPARAMETERIIVPROC gl_loader_glGetTexParameterIiv = NULL;
PFNGLGETTEXPARAMETERIUIVPROC gl_loader_glGetTexParameterIuiv = NULL;
PFNGLGETTEXPARAMETERFVPROC gl_loader_glGetTexParameterfv = NULL;
PFNGLGETTEXPARAMETERIVPROC gl_loader_glGetTexParameteriv = NULL;
PFNGLGETTEXTUREIMAGEPROC gl_loader_glGetTextureImage = NULL;
PFNGLGETTEXTURELEVELPARAMETERFVPROC gl_loader_glGetTextureLevelParameterfv = NULL;
PFNGLGETTEXTURELEVELPARAMETERIVPROC gl_loader_glGetTextureLevelParameteriv = NULL;
PFNGLGETTEXTUREPARAMETERIIVPROC gl_loader_glGetTextureParameterIiv = NULL;
PFNGLGETTEXTUREPARAMETERIUIVPROC gl_loader_glGetTextureParameterIuiv = NULL;
PFNGLGETTEXTUREPARAMETERFVPROC gl_loader_glGetTextureParameterfv = NULL;
PFNGLGETTEXTUREPARAMETERIVPROC gl_loader_glGetTextureParameteriv = NULL;
PFNGLGETTEXTURESUBIMAGEPROC gl_loader_glGetTextureSubImage = NULL;
PFNGLGETTRANSFORMFEEDBACKVARYINGPROC gl_loader_glGetTransformFeedbackVarying = NULL;
PFNGLGETTRANSFORMFEEDBACKI64_VPROC gl_loader_glGetTransformFeedbacki64_v = NULL;
PFNGLGETTRANSFORMFEEDBACKI_VPROC gl_loader_glGetTransformFeedbacki_v = NULL;
PFNGLGETTRANSFORMFEEDBACKIVPROC gl_loader_glGetTransformFeedbackiv = NULL;
PFNGLGETUNIFORMBLOCKINDEXPROC gl_loader_glGetUniformBlockIndex = NULL;
PFNGLGETUNIFORMINDICESPROC gl_loader_glGetUniformIndices = NULL;
PFNGLGETUNIFORMLOCATIONPROC gl_loader_glGetUniformLocation = NULL;
PFNGLGETUNIFORMSUBROUTINEUIVPROC gl_loader_glGetUniformSubroutineuiv = NULL;
PFNGLGETUNIFORMDVPROC gl_loader_glGetUniformdv = NULL;
PFNGLGETUNIFORMFVPROC gl_loader_glGetUniformfv = NULL;
PFNGLGETUNIFORMIVPROC gl_loader_glGetUniformiv = NULL;
PFNGLGETUNIFORMUIVPROC gl_loader_glGetUniformuiv = NULL;
PFNGLGETVERTEXARRAYINDEXED64IVPROC gl_loader_glGetVertexArrayIndexed64iv = NULL;
PFNGLGETVERTEXARRAYINDEXEDIVPROC gl_loader_glGetVertexArrayIndexediv = NULL;
PFNGLGETVERTEXARRAYIVPROC gl_loader_glGetVertexArrayiv = NULL;
PFNGLGETVERTEXATTRIBIIVPROC gl_loader_glGetVertexAttribIiv = NULL;
PFNGLGETVERTEXATTRIBIUIVPROC gl_loader_glGetVertexAttribIuiv = NULL;
PFNGLGETVERTEXATTRIBLDVPROC gl_loader_glGetVertexAttribLdv = NULL;
PFNGLGETVERTEXATTRIBPOINTERVPROC gl_loader_glGetVertexAttribPointerv = NULL;
PFNGLGETVERTEXATTRIBDVPROC gl_loader_glGetVertexAttribdv = NULL;
PFNGLGETVERTEXATTRIBFVPROC gl_loader_glGetVertexAttribfv = NULL;
PFNGLGETVERTEXATTRIBIVPROC gl_loader_glGetVertexAttribiv = NULL;
PFNGLGETNCOLORTABLEPROC gl_loader_glGetnColorTable = NULL;
PFNGLGETNCOMPRESSEDTEXIMAGEPROC gl_loader_glGetnCompressedTexImage = NULL;
PFNGLGETNCONVOLUTIONFILTERPROC gl_loader_glGetnConvolutionFilter = NULL;
PFNGLGETNHISTOGRAMPROC gl_loader_glGetnHistogram = NULL;
PFNGLGETNMAPDVPROC gl_loader_glGetnMapdv = NULL;
PFNGLGETNMAPFVPROC gl_loader_glGetnMapfv = NULL;
PFNGLGETNMAPIVPROC gl_loader_glGetnMapiv = NULL;
PFNGLGETNMINMAXPROC gl_loader_glGetnMinmax = NULL;
PFNGLGETNPIXELMAPFVPROC gl_loader_glGetnPixelMapfv = NULL;
PFNGLGETNPIXELMAPUIVPROC gl_loader_glGetnPixelMapuiv = NULL;
PFNGLGETNPIXELMAPUSVPROC gl_loader_glGetnPixelMapusv = NULL;
PFNGLGETNPOLYGONSTIPPLEPROC gl_loader_glGetnPolygonStipple = NULL;
PFNGLGETNSEPARABLEFILTERPROC gl_loader_glGetnSeparableFilter = NULL;
PFNGLGETNTEXIMAGEPROC gl_loader_glGetnTexImage = NULL;
PFNGLGETNUNIFORMDVPROC gl_loader_glGetnUniformdv = NULL;
PFNGLGETNUNIFORMFVPROC gl_loader_glGetnUniformfv = NULL;
PFNGLGETNUNIFORMIVPROC gl_loader_glGetnUniformiv = NULL;
PFNGLGETNUNIFORMUIVPROC gl_loader_glGetnUniformuiv = NULL;
PFNGLHINTPROC gl_loader_glHint = NULL;
PFNGLINVALIDATEBUFFERDATAPROC gl_loader_glInvalidateBufferData = NULL;
PFNGLINVALIDATEBUFFERSUBDATAPROC gl_loader_glInvalidateBufferSubData = NULL;
PFNGLINVALIDATEFRAMEBUFFERPROC gl_loader_glInvalidateFramebuffer = NULL;
PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC gl_loader_glInvalidateNamedFramebufferData = NULL;
PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC gl_loader_glInvalidateNamedFramebufferSubData = NULL;
PFNGLINVALIDATESUBFRAMEBUFFERPROC gl_loader_glInvalidateSubFramebuffer = NULL;
PFNGLINVALIDATETEXIMAGEPROC gl_loader_glInvalidateTexImage = NULL;
PFNGLINVALIDATETEXSUBIMAGEPROC gl_loader_glInvalidateTexSubImage = NULL;
PFNGLISBUFFERPROC gl_loader_glIsBuffer = NULL;
PFNGLISENABLEDPROC gl_loader_glIsEnabled = NULL;
PFNGLISENABLEDIPROC gl_loader_glIsEnabledi = NULL;
PFNGLISFRAMEBUFFERPROC gl_loader_glIsFramebuffer = NULL;
PFNGLISPROGRAMPROC gl_loader_glIsProgram = NULL;
PFNGLISPROGRAMPIPELINEPROC gl_loader_glIsProgramPipeline = NULL;
PFNGLISQUERYPROC gl_loader_glIsQuery = NULL;
PFNGLISRENDERBUFFERPROC gl_loader_glIsRenderbuffer = NULL;
PFNGLISSAMPLERPROC gl_loader_glIsSampler = NULL;
PFNGLISSHADERPROC gl_loader_glIsShader = NULL;
PFNGLISSYNCPROC gl_loader_glIsSync = NULL;
PFNGLISTEXTUREPROC gl_loader_glIsTexture = NULL;
PFNGLISTRANSFORMFEEDBACKPROC gl_loader_glIsTransformFeedback = NULL;
PFNGLISVERTEXARRAYPROC gl_loader_glIsVertexArray = NULL;
PFNGLLINEWIDTHPROC gl_loader_glLineWidth = NULL;
PFNGLLINKPROGRAMPROC gl_loader_glLinkProgram = NULL;
PFNGLLOGICOPPROC gl_loader_glLogicOp = NULL;
PFNGLMAPBUFFERPROC gl_loader_glMapBuffer = NULL;
PFNGLMAPBUFFERRANGEPROC gl_loader_glMapBufferRange = NULL;
PFNGLMAPNAMEDBUFFERPROC gl_loader_glMapNamedBuffer = NULL;
PFNGLMAPNAMEDBUFFERRANGEPROC gl_loader_glMapNamedBufferRange = NULL;
PFNGLMEMORYBARRIERPROC gl_loader_glMemoryBarrier = NULL;
PFNGLMEMORYBARRIERBYREGIONPROC gl_loader_glMemoryBarrierByRegion = NULL;
PFNGLMINSAMPLESHADINGPROC gl_loader_glMinSampleShading = NULL;
PFNGLMULTIDRAWARRAYSPROC gl_loader_glMultiDrawArrays = NULL;
PFNGLMULTIDRAWARRAYSINDIRECTPROC gl_loader_glMultiDrawArraysIndirect = NULL;
PFNGLMULTIDRAWELEMENTSPROC gl_loader_glMultiDrawElements = NULL;
PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC gl_loader_glMultiDrawElementsBaseVertex = NULL;
PFNGLMULTIDRAWELEMENTSINDIRECTPROC gl_loader_glMultiDrawElementsIndirect = NULL;
PFNGLMULTITEXCOORDP1UIPROC gl_loader_glMultiTexCoordP1ui = NULL;
PFNGLMULTITEXCOORDP1UIVPROC gl_loader_glMultiTexCoordP1uiv = NULL;
PFNGLMULTITEXCOORDP2UIPROC gl_loader_glMultiTexCoordP2ui = NULL;
PFNGLMULTITEXCOORDP2UIVPROC gl_loader_glMultiTexCoordP2uiv = NULL;
PFNGLMULTITEXCOORDP3UIPROC gl_loader_glMultiTexCoordP3ui = NULL;
PFNGLMULTITEXCOORDP3UIVPROC gl_loader_glMultiTexCoordP3uiv = NULL;
PFNGLMULTITEXCOORDP4UIPROC gl_loader_glMultiTexCoordP4ui = NULL;
PFNGLMULTITEXCOORDP4UIVPROC gl_loader_glMultiTexCoordP4uiv = NULL;
PFNGLNAMEDBUFFERDATAPROC gl_loader_glNamedBufferData = NULL;
PFNGLNAMEDBUFFERSTORAGEPROC gl_loader_glNamedBufferStorage = NULL;
PFNGLNAMEDBUFFERSUBDATAPROC gl_loader_glNamedBufferSubData = NULL;
PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC gl_loader_glNamedFramebufferDrawBuffer = NULL;
PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC gl_loader_glNamedFramebufferDrawBuffers = NULL;
PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC gl_loader_glNamedFramebufferParameteri = NULL;
PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC gl_loader_glNamedFramebufferReadBuffer = NULL;
PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC gl_loader_glNamedFramebufferRenderbuffer = NULL;
PFNGLNAMEDFRAMEBUFFERTEXTUREPROC gl_loader_glNamedFramebufferTexture = NULL;
PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC gl_loader_glNamedFramebufferTextureLayer = NULL;
PFNGLNAMEDRENDERBUFFERSTORAGEPROC gl_loader_glNamedRenderbufferStorage = NULL;
PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC gl_loader_glNamedRenderbufferStorageMultisample = NULL;
PFNGLNORMALP3UIPROC gl_loader_glNormalP3ui = NULL;
PFNGLNORMALP3UIVPROC gl_loader_glNormalP3uiv = NULL;
PFNGLOBJECTLABELPROC gl_loader_glObjectLabel = NULL;
PFNGLOBJECTPTRLABELPROC gl_loader_glObjectPtrLabel = NULL;
PFNGLPATCHPARAMETERFVPROC gl_loader_glPatchParameterfv = NULL;
PFNGLPATCHPARAMETERIPROC gl_loader_glPatchParameteri = NULL;
PFNGLPAUSETRANSFORMFEEDBACKPROC gl_loader_glPauseTransformFeedback = NULL;
PFNGLPIXELSTOREFPROC gl_loader_glPixelStoref = NULL;
PFNGLPIXELSTOREIPROC gl_loader_glPixelStorei = NULL;
PFNGLPOINTPARAMETERFPROC gl_loader_glPointParameterf = NULL;
PFNGLPOINTPARAMETERFVPROC gl_loader_glPointParameterfv = NULL;
PFNGLPOINTPARAMETERIPROC gl_loader_glPointParameteri = NULL;
PFNGLPOINTPARAMETERIVPROC gl_loader_glPointParameteriv = NULL;
PFNGLPOINTSIZEPROC gl_loader_glPointSize = NULL;
PFNGLPOLYGONMODEPROC gl_loader_glPolygonMode = NULL;
PFNGLPOLYGONOFFSETPROC gl_loader_glPolygonOffset = NULL;
PFNGLPOPDEBUGGROUPPROC gl_loader_glPopDebugGroup = NULL;
PFNGLPRIMITIVERESTARTINDEXPROC gl_loader_glPrimitiveRestartIndex = NULL;
PFNGLPROGRAMBINARYPROC gl_loader_glProgramBinary = NULL;
PFNGLPROGRAMPARAMETERIPROC gl_loader_glProgramParameteri = NULL;
PFNGLPROGRAMUNIFORM1DPROC gl_loader_glProgramUniform1d = NULL;
PFNGLPROGRAMUNIFORM1DVPROC gl_loader_glProgramUniform1dv = NULL;
PFNGLPROGRAMUNIFORM1FPROC gl_loader_glProgramUniform1f = NULL;
PFNGLPROGRAMUNIFORM1FVPROC gl_loader_glProgramUniform1fv = NULL;
PFNGLPROGRAMUNIFORM1IPROC gl_loader_glProgramUniform1i = NULL;
PFNGLPROGRAMUNIFORM1IVPROC gl_loader_glProgramUniform1iv = NULL;
PFNGLPROGRAMUNIFORM1UIPROC gl_loader_glProgramUniform1ui = NULL;
PFNGLPROGRAMUNIFORM1UIVPROC gl_loader_glProgramUniform1uiv = NULL;
PFNGLPROGRAMUNIFORM2DPROC gl_loader_glProgramUniform2d = NULL;
PFNGLPROGRAMUNIFORM2DVPROC gl_loader_glProgramUniform2dv = NULL;
PFNGLPROGRAMUNIFORM2FPROC gl_loader_glProgramUniform2f = NULL;
PFNGLPROGRAMUNIFORM2FVPROC gl_loader_glProgramUniform2fv = NULL;
PFNGLPROGRAMUNIFORM2IPROC gl_loader_glProgramUniform2i = NULL;
PFNGLPROGRAMUNIFORM2IVPROC gl_loader_glProgramUniform2iv = NULL;
PFNGLPROGRAMUNIFORM2UIPROC gl_loader_glProgramUniform2ui = NULL;
PFNGLPROGRAMUNIFORM2UIVPROC gl_loader_glProgramUniform2uiv = NULL;
PFNGLPROGRAMUNIFORM3DPROC gl_loader_glProgramUniform3d = NULL;
PFNGLPROGRAMUNIFORM3DVPROC gl_loader_glProgramUniform3dv = NULL;
PFNGLPROGRAMUNIFORM3FPROC gl_loader_glProgramUniform3f = NULL;
PFNGLPROGRAMUNIFORM3FVPROC gl_loader_glProgramUniform3fv = NULL;
PFNGLPROGRAMUNIFORM3IPROC gl_loader_glProgramUniform3i = NULL;
PFNGLPROGRAMUNIFORM3IVPROC gl_loader_glProgramUniform3iv = NULL;
PFNGLPROGRAMUNIFORM3UIPROC gl_loader_glProgramUniform3ui = NULL;
PFNGLPROGRAMUNIFORM3UIVPROC gl_loader_glProgramUniform3uiv = NULL;
PFNGLPROGRAMUNIFORM4DPROC gl_loader_glProgramUniform4d = NULL;
PFNGLPROGRAMUNIFORM4DVPROC gl_loader_glProgramUniform4dv = NULL;
PFNGLPROGRAMUNIFORM4FPROC gl_loader_glProgramUniform4f = NULL;
PFNGLPROGRAMUNIFORM4FVPROC gl_loader_glProgramUniform4fv = NULL;
PFNGLPROGRAMUNIFORM4IPROC gl_loader_glProgramUniform4i = NULL;
PFNGLPROGRAMUNIFORM4IVPROC gl_loader_glProgramUniform4iv = NULL;
PFNGLPROGRAMUNIFORM4UIPROC gl_loader_glProgramUniform4ui = NULL;
PFNGLPROGRAMUNIFORM4UIVPROC gl_loader_glProgramUniform4uiv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2DVPROC gl_loader_glProgramUniformMatrix2dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2FVPROC gl_loader_glProgramUniformMatrix2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC gl_loader_glProgramUniformMatrix2x3dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC gl_loader_glProgramUniformMatrix2x3fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC gl_loader_glProgramUniformMatrix2x4dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC gl_loader_glProgramUniformMatrix2x4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3DVPROC gl_loader_glProgramUniformMatrix3dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3FVPROC gl_loader_glProgramUniformMatrix3fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC gl_loader_glProgramUniformMatrix3x2dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC gl_loader_glProgramUniformMatrix3x2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC gl_loader_glProgramUniformMatrix3x4dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC gl_loader_glProgramUniformMatrix3x4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4DVPROC gl_loader_glProgramUniformMatrix4dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4FVPROC gl_loader_glProgramUniformMatrix4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC gl_loader_glProgramUniformMatrix4x2dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC gl_loader_glProgramUniformMatrix4x2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC gl_loader_glProgramUniformMatrix4x3dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC gl_loader_glProgramUniformMatrix4x3fv = NULL;
PFNGLPROVOKINGVERTEXPROC gl_loader_glProvokingVertex = NULL;
PFNGLPUSHDEBUGGROUPPROC gl_loader_glPushDebugGroup = NULL;
PFNGLQUERYCOUNTERPROC gl_loader_glQueryCounter = NULL;
PFNGLREADBUFFERPROC gl_loader_glReadBuffer = NULL;
PFNGLREADPIXELSPROC gl_loader_glReadPixels = NULL;
PFNGLREADNPIXELSPROC gl_loader_glReadnPixels = NULL;
PFNGLRELEASESHADERCOMPILERPROC gl_loader_glReleaseShaderCompiler = NULL;
PFNGLRENDERBUFFERSTORAGEPROC gl_loader_glRenderbufferStorage = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC gl_loader_glRenderbufferStorageMultisample = NULL;
PFNGLRESUMETRANSFORMFEEDBACKPROC gl_loader_glResumeTransformFeedback = NULL;
PFNGLSAMPLECOVERAGEPROC gl_loader_glSampleCoverage = NULL;
PFNGLSAMPLEMASKIPROC gl_loader_glSampleMaski = NULL;
PFNGLSAMPLERPARAMETERIIVPROC gl_loader_glSamplerParameterIiv = NULL;
PFNGLSAMPLERPARAMETERIUIVPROC gl_loader_glSamplerParameterIuiv = NULL;
PFNGLSAMPLERPARAMETERFPROC gl_loader_glSamplerParameterf = NULL;
PFNGLSAMPLERPARAMETERFVPROC gl_loader_glSamplerParameterfv = NULL;
PFNGLSAMPLERPARAMETERIPROC gl_loader_glSamplerParameteri = NULL;
PFNGLSAMPLERPARAMETERIVPROC gl_loader_glSamplerParameteriv = NULL;
PFNGLSCISSORPROC gl_loader_glScissor = NULL;
PFNGLSCISSORARRAYVPROC gl_loader_glScissorArrayv = NULL;
PFNGLSCISSORINDEXEDPROC gl_loader_glScissorIndexed = NULL;
PFNGLSCISSORINDEXEDVPROC gl_loader_glScissorIndexedv = NULL;
PFNGLSECONDARYCOLORP3UIPROC gl_loader_glSecondaryColorP3ui = NULL;
PFNGLSECONDARYCOLORP3UIVPROC gl_loader_glSecondaryColorP3uiv = NULL;
PFNGLSHADERBINARYPROC gl_loader_glShaderBinary = NULL;
PFNGLSHADERSOURCEPROC gl_loader_glShaderSource = NULL;
PFNGLSHADERSTORAGEBLOCKBINDINGPROC gl_loader_glShaderStorageBlockBinding = NULL;
PFNGLSTENCILFUNCPROC gl_loader_glStencilFunc = NULL;
PFNGLSTENCILFUNCSEPARATEPROC gl_loader_glStencilFuncSeparate = NULL;
PFNGLSTENCILMASKPROC gl_loader_glStencilMask = NULL;
PFNGLSTENCILMASKSEPARATEPROC gl_loader_glStencilMaskSeparate = NULL;
PFNGLSTENCILOPPROC gl_loader_glStencilOp = NULL;
PFNGLSTENCILOPSEPARATEPROC gl_loader_glStencilOpSeparate = NULL;
PFNGLTEXBUFFERPROC gl_loader_glTexBuffer = NULL;
PFNGLTEXBUFFERRANGEPROC gl_loader_glTexBufferRange = NULL;
PFNGLTEXCOORDP1UIPROC gl_loader_glTexCoordP1ui = NULL;
PFNGLTEXCOORDP1UIVPROC gl_loader_glTexCoordP1uiv = NULL;
PFNGLTEXCOORDP2UIPROC gl_loader_glTexCoordP2ui = NULL;
PFNGLTEXCOORDP2UIVPROC gl_loader_glTexCoordP2uiv = NULL;
PFNGLTEXCOORDP3UIPROC gl_loader_glTexCoordP3ui = NULL;
PFNGLTEXCOORDP3UIVPROC gl_loader_glTexCoordP3uiv = NULL;
PFNGLTEXCOORDP4UIPROC gl_loader_glTexCoordP4ui = NULL;
PFNGLTEXCOORDP4UIVPROC gl_loader_glTexCoordP4uiv = NULL;
PFNGLTEXIMAGE1DPROC gl_loader_glTexImage1D = NULL;
PFNGLTEXIMAGE2DPROC gl_loader_glTexImage2D = NULL;
PFNGLTEXIMAGE2DMULTISAMPLEPROC gl_loader_glTexImage2DMultisample = NULL;
PFNGLTEXIMAGE3DPROC gl_loader_glTexImage3D = NULL;
PFNGLTEXIMAGE3DMULTISAMPLEPROC gl_loader_glTexImage3DMultisample = NULL;
PFNGLTEXPARAMETERIIVPROC gl_loader_glTexParameterIiv = NULL;
PFNGLTEXPARAMETERIUIVPROC gl_loader_glTexParameterIuiv = NULL;
PFNGLTEXPARAMETERFPROC gl_loader_glTexParameterf = NULL;
PFNGLTEXPARAMETERFVPROC gl_loader_glTexParameterfv = NULL;
PFNGLTEXPARAMETERIPROC gl_loader_glTexParameteri = NULL;
PFNGLTEXPARAMETERIVPROC gl_loader_glTexParameteriv = NULL;
PFNGLTEXSTORAGE1DPROC gl_loader_glTexStorage1D = NULL;
PFNGLTEXSTORAGE2DPROC gl_loader_glTexStorage2D = NULL;
PFNGLTEXSTORAGE2DMULTISAMPLEPROC gl_loader_glTexStorage2DMultisample = NULL;
PFNGLTEXSTORAGE3DPROC gl_loader_glTexStorage3D = NULL;
PFNGLTEXSTORAGE3DMULTISAMPLEPROC gl_loader_glTexStorage3DMultisample = NULL;
PFNGLTEXSUBIMAGE1DPROC gl_loader_glTexSubImage1D = NULL;
PFNGLTEXSUBIMAGE2DPROC gl_loader_glTexSubImage2D = NULL;
PFNGLTEXSUBIMAGE3DPROC gl_loader_glTexSubImage3D = NULL;
PFNGLTEXTUREBARRIERPROC gl_loader_glTextureBarrier = NULL;
PFNGLTEXTUREBUFFERPROC gl_loader_glTextureBuffer = NULL;
PFNGLTEXTUREBUFFERRANGEPROC gl_loader_glTextureBufferRange = NULL;
PFNGLTEXTUREPARAMETERIIVPROC gl_loader_glTextureParameterIiv = NULL;
PFNGLTEXTUREPARAMETERIUIVPROC gl_loader_glTextureParameterIuiv = NULL;
PFNGLTEXTUREPARAMETERFPROC gl_loader_glTextureParameterf = NULL;
PFNGLTEXTUREPARAMETERFVPROC gl_loader_glTextureParameterfv = NULL;
PFNGLTEXTUREPARAMETERIPROC gl_loader_glTextureParameteri = NULL;
PFNGLTEXTUREPARAMETERIVPROC gl_loader_glTextureParameteriv = NULL;
PFNGLTEXTURESTORAGE1DPROC gl_loader_glTextureStorage1D = NULL;
PFNGLTEXTURESTORAGE2DPROC gl_loader_glTextureStorage2D = NULL;
PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC gl_loader_glTextureStorage2DMultisample = NULL;
PFNGLTEXTURESTORAGE3DPROC gl_loader_glTextureStorage3D = NULL;
PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC gl_loader_glTextureStorage3DMultisample = NULL;
PFNGLTEXTURESUBIMAGE1DPROC gl_loader_glTextureSubImage1D = NULL;
PFNGLTEXTURESUBIMAGE2DPROC gl_loader_glTextureSubImage2D = NULL;
PFNGLTEXTURESUBIMAGE3DPROC gl_loader_glTextureSubImage3D = NULL;
PFNGLTEXTUREVIEWPROC gl_loader_glTextureView = NULL;
PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC gl_loader_glTransformFeedbackBufferBase = NULL;
PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC gl_loader_glTransformFeedbackBufferRange = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC gl_loader_glTransformFeedbackVaryings = NULL;
PFNGLUNIFORM1DPROC gl_loader_glUniform1d = NULL;
PFNGLUNIFORM1DVPROC gl_loader_glUniform1dv = NULL;
PFNGLUNIFORM1FPROC gl_loader_glUniform1f = NULL;
PFNGLUNIFORM1FVPROC gl_loader_glUniform1fv = NULL;
PFNGLUNIFORM1IPROC gl_loader_glUniform1i = NULL;
PFNGLUNIFORM1IVPROC gl_loader_glUniform1iv = NULL;
PFNGLUNIFORM1UIPROC gl_loader_glUniform1ui = NULL;
PFNGLUNIFORM1UIVPROC gl_loader_glUniform1uiv = NULL;
PFNGLUNIFORM2DPROC gl_loader_glUniform2d = NULL;
PFNGLUNIFORM2DVPROC gl_loader_glUniform2dv = NULL;
PFNGLUNIFORM2FPROC gl_loader_glUniform2f = NULL;
PFNGLUNIFORM2FVPROC gl_loader_glUniform2fv = NULL;
PFNGLUNIFORM2IPROC gl_loader_glUniform2i = NULL;
PFNGLUNIFORM2IVPROC gl_loader_glUniform2iv = NULL;
PFNGLUNIFORM2UIPROC gl_loader_glUniform2ui = NULL;
PFNGLUNIFORM2UIVPROC gl_loader_glUniform2uiv = NULL;
PFNGLUNIFORM3DPROC gl_loader_glUniform3d = NULL;
PFNGLUNIFORM3DVPROC gl_loader_glUniform3dv = NULL;
PFNGLUNIFORM3FPROC gl_loader_glUniform3f = NULL;
PFNGLUNIFORM3FVPROC gl_loader_glUniform3fv = NULL;
PFNGLUNIFORM3IPROC gl_loader_glUniform3i = NULL;
PFNGLUNIFORM3IVPROC gl_loader_glUniform3iv = NULL;
PFNGLUNIFORM3UIPROC gl_loader_glUniform3ui = NULL;
PFNGLUNIFORM3UIVPROC gl_loader_glUniform3uiv = NULL;
PFNGLUNIFORM4DPROC gl_loader_glUniform4d = NULL;
PFNGLUNIFORM4DVPROC gl_loader_glUniform4dv = NULL;
PFNGLUNIFORM4FPROC gl_loader_glUniform4f = NULL;
PFNGLUNIFORM4FVPROC gl_loader_glUniform4fv = NULL;
PFNGLUNIFORM4IPROC gl_loader_glUniform4i = NULL;
PFNGLUNIFORM4IVPROC gl_loader_glUniform4iv = NULL;
PFNGLUNIFORM4UIPROC gl_loader_glUniform4ui = NULL;
PFNGLUNIFORM4UIVPROC gl_loader_glUniform4uiv = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC gl_loader_glUniformBlockBinding = NULL;
PFNGLUNIFORMMATRIX2DVPROC gl_loader_glUniformMatrix2dv = NULL;
PFNGLUNIFORMMATRIX2FVPROC gl_loader_glUniformMatrix2fv = NULL;
PFNGLUNIFORMMATRIX2X3DVPROC gl_loader_glUniformMatrix2x3dv = NULL;
PFNGLUNIFORMMATRIX2X3FVPROC gl_loader_glUniformMatrix2x3fv = NULL;
PFNGLUNIFORMMATRIX2X4DVPROC gl_loader_glUniformMatrix2x4dv = NULL;
PFNGLUNIFORMMATRIX2X4FVPROC gl_loader_glUniformMatrix2x4fv = NULL;
PFNGLUNIFORMMATRIX3DVPROC gl_loader_glUniformMatrix3dv = NULL;
PFNGLUNIFORMMATRIX3FVPROC gl_loader_glUniformMatrix3fv = NULL;
PFNGLUNIFORMMATRIX3X2DVPROC gl_loader_glUniformMatrix3x2dv = NULL;
PFNGLUNIFORMMATRIX3X2FVPROC gl_loader_glUniformMatrix3x2fv = NULL;
PFNGLUNIFORMMATRIX3X4DVPROC gl_loader_glUniformMatrix3x4dv = NULL;
PFNGLUNIFORMMATRIX3X4FVPROC gl_loader_glUniformMatrix3x4fv = NULL;
PFNGLUNIFORMMATRIX4DVPROC gl_loader_glUniformMatrix4dv = NULL;
PFNGLUNIFORMMATRIX4FVPROC gl_loader_glUniformMatrix4fv = NULL;
PFNGLUNIFORMMATRIX4X2DVPROC gl_loader_glUniformMatrix4x2dv = NULL;
PFNGLUNIFORMMATRIX4X2FVPROC gl_loader_glUniformMatrix4x2fv = NULL;
PFNGLUNIFORMMATRIX4X3DVPROC gl_loader_glUniformMatrix4x3dv = NULL;
PFNGLUNIFORMMATRIX4X3FVPROC gl_loader_glUniformMatrix4x3fv = NULL;
PFNGLUNIFORMSUBROUTINESUIVPROC gl_loader_glUniformSubroutinesuiv = NULL;
PFNGLUNMAPBUFFERPROC gl_loader_glUnmapBuffer = NULL;
PFNGLUNMAPNAMEDBUFFERPROC gl_loader_glUnmapNamedBuffer = NULL;
PFNGLUSEPROGRAMPROC gl_loader_glUseProgram = NULL;
PFNGLUSEPROGRAMSTAGESPROC gl_loader_glUseProgramStages = NULL;
PFNGLVALIDATEPROGRAMPROC gl_loader_glValidateProgram = NULL;
PFNGLVALIDATEPROGRAMPIPELINEPROC gl_loader_glValidateProgramPipeline = NULL;
PFNGLVERTEXARRAYATTRIBBINDINGPROC gl_loader_glVertexArrayAttribBinding = NULL;
PFNGLVERTEXARRAYATTRIBFORMATPROC gl_loader_glVertexArrayAttribFormat = NULL;
PFNGLVERTEXARRAYATTRIBIFORMATPROC gl_loader_glVertexArrayAttribIFormat = NULL;
PFNGLVERTEXARRAYATTRIBLFORMATPROC gl_loader_glVertexArrayAttribLFormat = NULL;
PFNGLVERTEXARRAYBINDINGDIVISORPROC gl_loader_glVertexArrayBindingDivisor = NULL;
PFNGLVERTEXARRAYELEMENTBUFFERPROC gl_loader_glVertexArrayElementBuffer = NULL;
PFNGLVERTEXARRAYVERTEXBUFFERPROC gl_loader_glVertexArrayVertexBuffer = NULL;
PFNGLVERTEXARRAYVERTEXBUFFERSPROC gl_loader_glVertexArrayVertexBuffers = NULL;
PFNGLVERTEXATTRIB1DPROC gl_loader_glVertexAttrib1d = NULL;
PFNGLVERTEXATTRIB1DVPROC gl_loader_glVertexAttrib1dv = NULL;
PFNGLVERTEXATTRIB1FPROC gl_loader_glVertexAttrib1f = NULL;
PFNGLVERTEXATTRIB1FVPROC gl_loader_glVertexAttrib1fv = NULL;
PFNGLVERTEXATTRIB1SPROC gl_loader_glVertexAttrib1s = NULL;
PFNGLVERTEXATTRIB1SVPROC gl_loader_glVertexAttrib1sv = NULL;
PFNGLVERTEXATTRIB2DPROC gl_loader_glVertexAttrib2d = NULL;
PFNGLVERTEXATTRIB2DVPROC gl_loader_glVertexAttrib2dv = NULL;
PFNGLVERTEXATTRIB2FPROC gl_loader_glVertexAttrib2f = NULL;
PFNGLVERTEXATTRIB2FVPROC gl_loader_glVertexAttrib2fv = NULL;
PFNGLVERTEXATTRIB2SPROC gl_loader_glVertexAttrib2s = NULL;
PFNGLVERTEXATTRIB2SVPROC gl_loader_glVertexAttrib2sv = NULL;
PFNGLVERTEXATTRIB3DPROC gl_loader_glVertexAttrib3d = NULL;
PFNGLVERTEXATTRIB3DVPROC gl_loader_glVertexAttrib3dv = NULL;
PFNGLVERTEXATTRIB3FPROC gl_loader_glVertexAttrib3f = NULL;
PFNGLVERTEXATTRIB3FVPROC gl_loader_glVertexAttrib3fv = NULL;
PFNGLVERTEXATTRIB3SPROC gl_loader_glVertexAttrib3s = NULL;
PFNGLVERTEXATTRIB3SVPROC gl_loader_glVertexAttrib3sv = NULL;
PFNGLVERTEXATTRIB4NBVPROC gl_loader_glVertexAttrib4Nbv = NULL;
PFNGLVERTEXATTRIB4NIVPROC gl_loader_glVertexAttrib4Niv = NULL;
PFNGLVERTEXATTRIB4NSVPROC gl_loader_glVertexAttrib4Nsv = NULL;
PFNGLVERTEXATTRIB4NUBPROC gl_loader_glVertexAttrib4Nub = NULL;
PFNGLVERTEXATTRIB4NUBVPROC gl_loader_glVertexAttrib4Nubv = NULL;
PFNGLVERTEXATTRIB4NUIVPROC gl_loader_glVertexAttrib4Nuiv = NULL;
PFNGLVERTEXATTRIB4NUSVPROC gl_loader_glVertexAttrib4Nusv = NULL;
PFNGLVERTEXATTRIB4BVPROC gl_loader_glVertexAttrib4bv = NULL;
PFNGLVERTEXATTRIB4DPROC gl_loader_glVertexAttrib4d = NULL;
PFNGLVERTEXATTRIB4DVPROC gl_loader_glVertexAttrib4dv = NULL;
PFNGLVERTEXATTRIB4FPROC gl_loader_glVertexAttrib4f = NULL;
PFNGLVERTEXATTRIB4FVPROC gl_loader_glVertexAttrib4fv = NULL;
PFNGLVERTEXATTRIB4IVPROC gl_loader_glVertexAttrib4iv = NULL;
PFNGLVERTEXATTRIB4SPROC gl_loader_glVertexAttrib4s = NULL;
PFNGLVERTEXATTRIB4SVPROC gl_loader_glVertexAttrib4sv = NULL;
PFNGLVERTEXATTRIB4UBVPROC gl_loader_glVertexAttrib4ubv = NULL;
PFNGLVERTEXATTRIB4UIVPROC gl_loader_glVertexAttrib4uiv = NULL;
PFNGLVERTEXATTRIB4USVPROC gl_loader_glVertexAttrib4usv = NULL;
PFNGLVERTEXATTRIBBINDINGPROC gl_loader_glVertexAttribBinding = NULL;
PFNGLVERTEXATTRIBDIVISORPROC gl_loader_glVertexAttribDivisor = NULL;
PFNGLVERTEXATTRIBFORMATPROC gl_loader_glVertexAttribFormat = NULL;
PFNGLVERTEXATTRIBI1IPROC gl_loader_glVertexAttribI1i = NULL;
PFNGLVERTEXATTRIBI1IVPROC gl_loader_glVertexAttribI1iv = NULL;
PFNGLVERTEXATTRIBI1UIPROC gl_loader_glVertexAttribI1ui = NULL;
PFNGLVERTEXATTRIBI1UIVPROC gl_loader_glVertexAttribI1uiv = NULL;
PFNGLVERTEXATTRIBI2IPROC gl_loader_glVertexAttribI2i = NULL;
PFNGLVERTEXATTRIBI2IVPROC gl_loader_glVertexAttribI2iv = NULL;
PFNGLVERTEXATTRIBI2UIPROC gl_loader_glVertexAttribI2ui = NULL;
PFNGLVERTEXATTRIBI2UIVPROC gl_loader_glVertexAttribI2uiv = NULL;
PFNGLVERTEXATTRIBI3IPROC gl_loader_glVertexAttribI3i = NULL;
PFNGLVERTEXATTRIBI3IVPROC gl_loader_glVertexAttribI3iv = NULL;
PFNGLVERTEXATTRIBI3UIPROC gl_loader_glVertexAttribI3ui = NULL;
PFNGLVERTEXATTRIBI3UIVPROC gl_loader_glVertexAttribI3uiv = NULL;
PFNGLVERTEXATTRIBI4BVPROC gl_loader_glVertexAttribI4bv = NULL;
PFNGLVERTEXATTRIBI4IPROC gl_loader_glVertexAttribI4i = NULL;
PFNGLVERTEXATTRIBI4IVPROC gl_loader_glVertexAttribI4iv = NULL;
PFNGLVERTEXATTRIBI4SVPROC gl_loader_glVertexAttribI4sv = NULL;
PFNGLVERTEXATTRIBI4UBVPROC gl_loader_glVertexAttribI4ubv = NULL;
PFNGLVERTEXATTRIBI4UIPROC gl_loader_glVertexAttribI4ui = NULL;
PFNGLVERTEXATTRIBI4UIVPROC gl_loader_glVertexAttribI4uiv = NULL;
PFNGLVERTEXATTRIBI4USVPROC gl_loader_glVertexAttribI4usv = NULL;
PFNGLVERTEXATTRIBIFORMATPROC gl_loader_glVertexAttribIFormat = NULL;
PFNGLVERTEXATTRIBIPOINTERPROC gl_loader_glVertexAttribIPointer = NULL;
PFNGLVERTEXATTRIBL1DPROC gl_loader_glVertexAttribL1d = NULL;
PFNGLVERTEXATTRIBL1DVPROC gl_loader_glVertexAttribL1dv = NULL;
PFNGLVERTEXATTRIBL2DPROC gl_loader_glVertexAttribL2d = NULL;
PFNGLVERTEXATTRIBL2DVPROC gl_loader_glVertexAttribL2dv = NULL;
PFNGLVERTEXATTRIBL3DPROC gl_loader_glVertexAttribL3d = NULL;
PFNGLVERTEXATTRIBL3DVPROC gl_loader_glVertexAttribL3dv = NULL;
PFNGLVERTEXATTRIBL4DPROC gl_loader_glVertexAttribL4d = NULL;
PFNGLVERTEXATTRIBL4DVPROC gl_loader_glVertexAttribL4dv = NULL;
PFNGLVERTEXATTRIBLFORMATPROC gl_loader_glVertexAttribLFormat = NULL;
PFNGLVERTEXATTRIBLPOINTERPROC gl_loader_glVertexAttribLPointer = NULL;
PFNGLVERTEXATTRIBP1UIPROC gl_loader_glVertexAttribP1ui = NULL;
PFNGLVERTEXATTRIBP1UIVPROC gl_loader_glVertexAttribP1uiv = NULL;
PFNGLVERTEXATTRIBP2UIPROC gl_loader_glVertexAttribP2ui = NULL;
PFNGLVERTEXATTRIBP2UIVPROC gl_loader_glVertexAttribP2uiv = NULL;
PFNGLVERTEXATTRIBP3UIPROC gl_loader_glVertexAttribP3ui = NULL;
PFNGLVERTEXATTRIBP3UIVPROC gl_loader_glVertexAttribP3uiv = NULL;
PFNGLVERTEXATTRIBP4UIPROC gl_loader_glVertexAttribP4ui = NULL;
PFNGLVERTEXATTRIBP4UIVPROC gl_loader_glVertexAttribP4uiv = NULL;
PFNGLVERTEXATTRIBPOINTERPROC gl_loader_glVertexAttribPointer = NULL;
PFNGLVERTEXBINDINGDIVISORPROC gl_loader_glVertexBindingDivisor = NULL;
PFNGLVERTEXP2UIPROC gl_loader_glVertexP2ui = NULL;
PFNGLVERTEXP2UIVPROC gl_loader_glVertexP2uiv = NULL;
PFNGLVERTEXP3UIPROC gl_loader_glVertexP3ui = NULL;
PFNGLVERTEXP3UIVPROC gl_loader_glVertexP3uiv = NULL;
PFNGLVERTEXP4UIPROC gl_loader_glVertexP4ui = NULL;
PFNGLVERTEXP4UIVPROC gl_loader_glVertexP4uiv = NULL;
PFNGLVIEWPORTPROC gl_loader_glViewport = NULL;
PFNGLVIEWPORTARRAYVPROC gl_loader_glViewportArrayv = NULL;
PFNGLVIEWPORTINDEXEDFPROC gl_loader_glViewportIndexedf = NULL;
PFNGLVIEWPORTINDEXEDFVPROC gl_loader_glViewportIndexedfv = NULL;
PFNGLWAITSYNCPROC gl_loader_glWaitSync = NULL;
static void load_GL_VERSION_1_0(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_1_0) return;
	gl_loader_glCullFace = (PFNGLCULLFACEPROC)load("glCullFace");
	gl_loader_glFrontFace = (PFNGLFRONTFACEPROC)load("glFrontFace");
	gl_loader_glHint = (PFNGLHINTPROC)load("glHint");
	gl_loader_glLineWidth = (PFNGLLINEWIDTHPROC)load("glLineWidth");
	gl_loader_glPointSize = (PFNGLPOINTSIZEPROC)load("glPointSize");
	gl_loader_glPolygonMode = (PFNGLPOLYGONMODEPROC)load("glPolygonMode");
	gl_loader_glScissor = (PFNGLSCISSORPROC)load("glScissor");
	gl_loader_glTexParameterf = (PFNGLTEXPARAMETERFPROC)load("glTexParameterf");
	gl_loader_glTexParameterfv = (PFNGLTEXPARAMETERFVPROC)load("glTexParameterfv");
	gl_loader_glTexParameteri = (PFNGLTEXPARAMETERIPROC)load("glTexParameteri");
	gl_loader_glTexParameteriv = (PFNGLTEXPARAMETERIVPROC)load("glTexParameteriv");
	gl_loader_glTexImage1D = (PFNGLTEXIMAGE1DPROC)load("glTexImage1D");
	gl_loader_glTexImage2D = (PFNGLTEXIMAGE2DPROC)load("glTexImage2D");
	gl_loader_glDrawBuffer = (PFNGLDRAWBUFFERPROC)load("glDrawBuffer");
	gl_loader_glClear = (PFNGLCLEARPROC)load("glClear");
	gl_loader_glClearColor = (PFNGLCLEARCOLORPROC)load("glClearColor");
	gl_loader_glClearStencil = (PFNGLCLEARSTENCILPROC)load("glClearStencil");
	gl_loader_glClearDepth = (PFNGLCLEARDEPTHPROC)load("glClearDepth");
	gl_loader_glStencilMask = (PFNGLSTENCILMASKPROC)load("glStencilMask");
	gl_loader_glColorMask = (PFNGLCOLORMASKPROC)load("glColorMask");
	gl_loader_glDepthMask = (PFNGLDEPTHMASKPROC)load("glDepthMask");
	gl_loader_glDisable = (PFNGLDISABLEPROC)load("glDisable");
	gl_loader_glEnable = (PFNGLENABLEPROC)load("glEnable");
	gl_loader_glFinish = (PFNGLFINISHPROC)load("glFinish");
	gl_loader_glFlush = (PFNGLFLUSHPROC)load("glFlush");
	gl_loader_glBlendFunc = (PFNGLBLENDFUNCPROC)load("glBlendFunc");
	gl_loader_glLogicOp = (PFNGLLOGICOPPROC)load("glLogicOp");
	gl_loader_glStencilFunc = (PFNGLSTENCILFUNCPROC)load("glStencilFunc");
	gl_loader_glStencilOp = (PFNGLSTENCILOPPROC)load("glStencilOp");
	gl_loader_glDepthFunc = (PFNGLDEPTHFUNCPROC)load("glDepthFunc");
	gl_loader_glPixelStoref = (PFNGLPIXELSTOREFPROC)load("glPixelStoref");
	gl_loader_glPixelStorei = (PFNGLPIXELSTOREIPROC)load("glPixelStorei");
	gl_loader_glReadBuffer = (PFNGLREADBUFFERPROC)load("glReadBuffer");
	gl_loader_glReadPixels = (PFNGLREADPIXELSPROC)load("glReadPixels");
	gl_loader_glGetBooleanv = (PFNGLGETBOOLEANVPROC)load("glGetBooleanv");
	gl_loader_glGetDoublev = (PFNGLGETDOUBLEVPROC)load("glGetDoublev");
	gl_loader_glGetError = (PFNGLGETERRORPROC)load("glGetError");
	gl_loader_glGetFloatv = (PFNGLGETFLOATVPROC)load("glGetFloatv");
	gl_loader_glGetIntegerv = (PFNGLGETINTEGERVPROC)load("glGetIntegerv");
	gl_loader_glGetString = (PFNGLGETSTRINGPROC)load("glGetString");
	gl_loader_glGetTexImage = (PFNGLGETTEXIMAGEPROC)load("glGetTexImage");
	gl_loader_glGetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC)load("glGetTexParameterfv");
	gl_loader_glGetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC)load("glGetTexParameteriv");
	gl_loader_glGetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFVPROC)load("glGetTexLevelParameterfv");
	gl_loader_glGetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIVPROC)load("glGetTexLevelParameteriv");
	gl_loader_glIsEnabled = (PFNGLISENABLEDPROC)load("glIsEnabled");
	gl_loader_glDepthRange = (PFNGLDEPTHRANGEPROC)load("glDepthRange");
	gl_loader_glViewport = (PFNGLVIEWPORTPROC)load("glViewport");
}
static void load_GL_VERSION_1_1(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_1_1) return;
	gl_loader_glDrawArrays = (PFNGLDRAWARRAYSPROC)load("glDrawArrays");
	gl_loader_glDrawElements = (PFNGLDRAWELEMENTSPROC)load("glDrawElements");
	gl_loader_glPolygonOffset = (PFNGLPOLYGONOFFSETPROC)load("glPolygonOffset");
	gl_loader_glCopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC)load("glCopyTexImage1D");
	gl_loader_glCopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC)load("glCopyTexImage2D");
	gl_loader_glCopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC)load("glCopyTexSubImage1D");
	gl_loader_glCopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC)load("glCopyTexSubImage2D");
	gl_loader_glTexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC)load("glTexSubImage1D");
	gl_loader_glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)load("glTexSubImage2D");
	gl_loader_glBindTexture = (PFNGLBINDTEXTUREPROC)load("glBindTexture");
	gl_loader_glDeleteTextures = (PFNGLDELETETEXTURESPROC)load("glDeleteTextures");
	gl_loader_glGenTextures = (PFNGLGENTEXTURESPROC)load("glGenTextures");
	gl_loader_glIsTexture = (PFNGLISTEXTUREPROC)load("glIsTexture");
}
static void load_GL_VERSION_1_2(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_1_2) return;
	gl_loader_glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)load("glDrawRangeElements");
	gl_loader_glTexImage3D = (PFNGLTEXIMAGE3DPROC)load("glTexImage3D");
	gl_loader_glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)load("glTexSubImage3D");
	gl_loader_glCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)load("glCopyTexSubImage3D");
}
static void load_GL_VERSION_1_3(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_1_3) return;
	gl_loader_glActiveTexture = (PFNGLACTIVETEXTUREPROC)load("glActiveTexture");
	gl_loader_glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC)load("glSampleCoverage");
	gl_loader_glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)load("glCompressedTexImage3D");
	gl_loader_glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)load("glCompressedTexImage2D");
	gl_loader_glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)load("glCompressedTexImage1D");
	gl_loader_glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)load("glCompressedTexSubImage3D");
	gl_loader_glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)load("glCompressedTexSubImage2D");
	gl_loader_glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)load("glCompressedTexSubImage1D");
	gl_loader_glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)load("glGetCompressedTexImage");
}
static void load_GL_VERSION_1_4(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_1_4) return;
	gl_loader_glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)load("glBlendFuncSeparate");
	gl_loader_glMultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)load("glMultiDrawArrays");
	gl_loader_glMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)load("glMultiDrawElements");
	gl_loader_glPointParameterf = (PFNGLPOINTPARAMETERFPROC)load("glPointParameterf");
	gl_loader_glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)load("glPointParameterfv");
	gl_loader_glPointParameteri = (PFNGLPOINTPARAMETERIPROC)load("glPointParameteri");
	gl_loader_glPointParameteriv = (PFNGLPOINTPARAMETERIVPROC)load("glPointParameteriv");
	gl_loader_glBlendColor = (PFNGLBLENDCOLORPROC)load("glBlendColor");
	gl_loader_glBlendEquation = (PFNGLBLENDEQUATIONPROC)load("glBlendEquation");
}
static void load_GL_VERSION_1_5(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_1_5) return;
	gl_loader_glGenQueries = (PFNGLGENQUERIESPROC)load("glGenQueries");
	gl_loader_glDeleteQueries = (PFNGLDELETEQUERIESPROC)load("glDeleteQueries");
	gl_loader_glIsQuery = (PFNGLISQUERYPROC)load("glIsQuery");
	gl_loader_glBeginQuery = (PFNGLBEGINQUERYPROC)load("glBeginQuery");
	gl_loader_glEndQuery = (PFNGLENDQUERYPROC)load("glEndQuery");
	gl_loader_glGetQueryiv = (PFNGLGETQUERYIVPROC)load("glGetQueryiv");
	gl_loader_glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)load("glGetQueryObjectiv");
	gl_loader_glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)load("glGetQueryObjectuiv");
	gl_loader_glBindBuffer = (PFNGLBINDBUFFERPROC)load("glBindBuffer");
	gl_loader_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)load("glDeleteBuffers");
	gl_loader_glGenBuffers = (PFNGLGENBUFFERSPROC)load("glGenBuffers");
	gl_loader_glIsBuffer = (PFNGLISBUFFERPROC)load("glIsBuffer");
	gl_loader_glBufferData = (PFNGLBUFFERDATAPROC)load("glBufferData");
	gl_loader_glBufferSubData = (PFNGLBUFFERSUBDATAPROC)load("glBufferSubData");
	gl_loader_glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)load("glGetBufferSubData");
	gl_loader_glMapBuffer = (PFNGLMAPBUFFERPROC)load("glMapBuffer");
	gl_loader_glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)load("glUnmapBuffer");
	gl_loader_glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)load("glGetBufferParameteriv");
	gl_loader_glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)load("glGetBufferPointerv");
}
static void load_GL_VERSION_2_0(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_2_0) return;
	gl_loader_glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)load("glBlendEquationSeparate");
	gl_loader_glDrawBuffers = (PFNGLDRAWBUFFERSPROC)load("glDrawBuffers");
	gl_loader_glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)load("glStencilOpSeparate");
	gl_loader_glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)load("glStencilFuncSeparate");
	gl_loader_glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)load("glStencilMaskSeparate");
	gl_loader_glAttachShader = (PFNGLATTACHSHADERPROC)load("glAttachShader");
	gl_loader_glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)load("glBindAttribLocation");
	gl_loader_glCompileShader = (PFNGLCOMPILESHADERPROC)load("glCompileShader");
	gl_loader_glCreateProgram = (PFNGLCREATEPROGRAMPROC)load("glCreateProgram");
	gl_loader_glCreateShader = (PFNGLCREATESHADERPROC)load("glCreateShader");
	gl_loader_glDeleteProgram = (PFNGLDELETEPROGRAMPROC)load("glDeleteProgram");
	gl_loader_glDeleteShader = (PFNGLDELETESHADERPROC)load("glDeleteShader");
	gl_loader_glDetachShader = (PFNGLDETACHSHADERPROC)load("glDetachShader");
	gl_loader_glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)load("glDisableVertexAttribArray");
	gl_loader_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)load("glEnableVertexAttribArray");
	gl_loader_glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)load("glGetActiveAttrib");
	gl_loader_glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)load("glGetActiveUniform");
	gl_loader_glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)load("glGetAttachedShaders");
	gl_loader_glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)load("glGetAttribLocation");
	gl_loader_glGetProgramiv = (PFNGLGETPROGRAMIVPROC)load("glGetProgramiv");
	gl_loader_glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)load("glGetProgramInfoLog");
	gl_loader_glGetShaderiv = (PFNGLGETSHADERIVPROC)load("glGetShaderiv");
	gl_loader_glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)load("glGetShaderInfoLog");
	gl_loader_glGetShaderSource = (PFNGLGETSHADERSOURCEPROC)load("glGetShaderSource");
	gl_loader_glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)load("glGetUniformLocation");
	gl_loader_glGetUniformfv = (PFNGLGETUNIFORMFVPROC)load("glGetUniformfv");
	gl_loader_glGetUniformiv = (PFNGLGETUNIFORMIVPROC)load("glGetUniformiv");
	gl_loader_glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)load("glGetVertexAttribdv");
	gl_loader_glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)load("glGetVertexAttribfv");
	gl_loader_glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)load("glGetVertexAttribiv");
	gl_loader_glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)load("glGetVertexAttribPointerv");
	gl_loader_glIsProgram = (PFNGLISPROGRAMPROC)load("glIsProgram");
	gl_loader_glIsShader = (PFNGLISSHADERPROC)load("glIsShader");
	gl_loader_glLinkProgram = (PFNGLLINKPROGRAMPROC)load("glLinkProgram");
	gl_loader_glShaderSource = (PFNGLSHADERSOURCEPROC)load("glShaderSource");
	gl_loader_glUseProgram = (PFNGLUSEPROGRAMPROC)load("glUseProgram");
	gl_loader_glUniform1f = (PFNGLUNIFORM1FPROC)load("glUniform1f");
	gl_loader_glUniform2f = (PFNGLUNIFORM2FPROC)load("glUniform2f");
	gl_loader_glUniform3f = (PFNGLUNIFORM3FPROC)load("glUniform3f");
	gl_loader_glUniform4f = (PFNGLUNIFORM4FPROC)load("glUniform4f");
	gl_loader_glUniform1i = (PFNGLUNIFORM1IPROC)load("glUniform1i");
	gl_loader_glUniform2i = (PFNGLUNIFORM2IPROC)load("glUniform2i");
	gl_loader_glUniform3i = (PFNGLUNIFORM3IPROC)load("glUniform3i");
	gl_loader_glUniform4i = (PFNGLUNIFORM4IPROC)load("glUniform4i");
	gl_loader_glUniform1fv = (PFNGLUNIFORM1FVPROC)load("glUniform1fv");
	gl_loader_glUniform2fv = (PFNGLUNIFORM2FVPROC)load("glUniform2fv");
	gl_loader_glUniform3fv = (PFNGLUNIFORM3FVPROC)load("glUniform3fv");
	gl_loader_glUniform4fv = (PFNGLUNIFORM4FVPROC)load("glUniform4fv");
	gl_loader_glUniform1iv = (PFNGLUNIFORM1IVPROC)load("glUniform1iv");
	gl_loader_glUniform2iv = (PFNGLUNIFORM2IVPROC)load("glUniform2iv");
	gl_loader_glUniform3iv = (PFNGLUNIFORM3IVPROC)load("glUniform3iv");
	gl_loader_glUniform4iv = (PFNGLUNIFORM4IVPROC)load("glUniform4iv");
	gl_loader_glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)load("glUniformMatrix2fv");
	gl_loader_glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)load("glUniformMatrix3fv");
	gl_loader_glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)load("glUniformMatrix4fv");
	gl_loader_glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)load("glValidateProgram");
	gl_loader_glVertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)load("glVertexAttrib1d");
	gl_loader_glVertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)load("glVertexAttrib1dv");
	gl_loader_glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)load("glVertexAttrib1f");
	gl_loader_glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)load("glVertexAttrib1fv");
	gl_loader_glVertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)load("glVertexAttrib1s");
	gl_loader_glVertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)load("glVertexAttrib1sv");
	gl_loader_glVertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)load("glVertexAttrib2d");
	gl_loader_glVertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)load("glVertexAttrib2dv");
	gl_loader_glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)load("glVertexAttrib2f");
	gl_loader_glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)load("glVertexAttrib2fv");
	gl_loader_glVertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)load("glVertexAttrib2s");
	gl_loader_glVertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)load("glVertexAttrib2sv");
	gl_loader_glVertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)load("glVertexAttrib3d");
	gl_loader_glVertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)load("glVertexAttrib3dv");
	gl_loader_glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)load("glVertexAttrib3f");
	gl_loader_glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)load("glVertexAttrib3fv");
	gl_loader_glVertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)load("glVertexAttrib3s");
	gl_loader_glVertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)load("glVertexAttrib3sv");
	gl_loader_glVertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)load("glVertexAttrib4Nbv");
	gl_loader_glVertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)load("glVertexAttrib4Niv");
	gl_loader_glVertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)load("glVertexAttrib4Nsv");
	gl_loader_glVertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)load("glVertexAttrib4Nub");
	gl_loader_glVertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)load("glVertexAttrib4Nubv");
	gl_loader_glVertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)load("glVertexAttrib4Nuiv");
	gl_loader_glVertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)load("glVertexAttrib4Nusv");
	gl_loader_glVertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)load("glVertexAttrib4bv");
	gl_loader_glVertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)load("glVertexAttrib4d");
	gl_loader_glVertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)load("glVertexAttrib4dv");
	gl_loader_glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)load("glVertexAttrib4f");
	gl_loader_glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)load("glVertexAttrib4fv");
	gl_loader_glVertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)load("glVertexAttrib4iv");
	gl_loader_glVertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)load("glVertexAttrib4s");
	gl_loader_glVertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)load("glVertexAttrib4sv");
	gl_loader_glVertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)load("glVertexAttrib4ubv");
	gl_loader_glVertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)load("glVertexAttrib4uiv");
	gl_loader_glVertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)load("glVertexAttrib4usv");
	gl_loader_glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)load("glVertexAttribPointer");
}
static void load_GL_VERSION_2_1(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_2_1) return;
	gl_loader_glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)load("glUniformMatrix2x3fv");
	gl_loader_glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)load("glUniformMatrix3x2fv");
	gl_loader_glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)load("glUniformMatrix2x4fv");
	gl_loader_glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)load("glUniformMatrix4x2fv");
	gl_loader_glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)load("glUniformMatrix3x4fv");
	gl_loader_glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)load("glUniformMatrix4x3fv");
}
static void load_GL_VERSION_3_0(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_3_0) return;
	gl_loader_glColorMaski = (PFNGLCOLORMASKIPROC)load("glColorMaski");
	gl_loader_glGetBooleani_v = (PFNGLGETBOOLEANI_VPROC)load("glGetBooleani_v");
	gl_loader_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)load("glGetIntegeri_v");
	gl_loader_glEnablei = (PFNGLENABLEIPROC)load("glEnablei");
	gl_loader_glDisablei = (PFNGLDISABLEIPROC)load("glDisablei");
	gl_loader_glIsEnabledi = (PFNGLISENABLEDIPROC)load("glIsEnabledi");
	gl_loader_glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)load("glBeginTransformFeedback");
	gl_loader_glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)load("glEndTransformFeedback");
	gl_loader_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)load("glBindBufferRange");
	gl_loader_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)load("glBindBufferBase");
	gl_loader_glTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)load("glTransformFeedbackVaryings");
	gl_loader_glGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)load("glGetTransformFeedbackVarying");
	gl_loader_glClampColor = (PFNGLCLAMPCOLORPROC)load("glClampColor");
	gl_loader_glBeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC)load("glBeginConditionalRender");
	gl_loader_glEndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC)load("glEndConditionalRender");
	gl_loader_glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)load("glVertexAttribIPointer");
	gl_loader_glGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)load("glGetVertexAttribIiv");
	gl_loader_glGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)load("glGetVertexAttribIuiv");
	gl_loader_glVertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC)load("glVertexAttribI1i");
	gl_loader_glVertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC)load("glVertexAttribI2i");
	gl_loader_glVertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC)load("glVertexAttribI3i");
	gl_loader_glVertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)load("glVertexAttribI4i");
	gl_loader_glVertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC)load("glVertexAttribI1ui");
	gl_loader_glVertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC)load("glVertexAttribI2ui");
	gl_loader_glVertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC)load("glVertexAttribI3ui");
	gl_loader_glVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)load("glVertexAttribI4ui");
	gl_loader_glVertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC)load("glVertexAttribI1iv");
	gl_loader_glVertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC)load("glVertexAttribI2iv");
	gl_loader_glVertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC)load("glVertexAttribI3iv");
	gl_loader_glVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)load("glVertexAttribI4iv");
	gl_loader_glVertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC)load("glVertexAttribI1uiv");
	gl_loader_glVertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC)load("glVertexAttribI2uiv");
	gl_loader_glVertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC)load("glVertexAttribI3uiv");
	gl_loader_glVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)load("glVertexAttribI4uiv");
	gl_loader_glVertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC)load("glVertexAttribI4bv");
	gl_loader_glVertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC)load("glVertexAttribI4sv");
	gl_loader_glVertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC)load("glVertexAttribI4ubv");
	gl_loader_glVertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC)load("glVertexAttribI4usv");
	gl_loader_glGetUniformuiv = (PFNGLGETUNIFORMUIVPROC)load("glGetUniformuiv");
	gl_loader_glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)load("glBindFragDataLocation");
	gl_loader_glGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)load("glGetFragDataLocation");
	gl_loader_glUniform1ui = (PFNGLUNIFORM1UIPROC)load("glUniform1ui");
	gl_loader_glUniform2ui = (PFNGLUNIFORM2UIPROC)load("glUniform2ui");
	gl_loader_glUniform3ui = (PFNGLUNIFORM3UIPROC)load("glUniform3ui");
	gl_loader_glUniform4ui = (PFNGLUNIFORM4UIPROC)load("glUniform4ui");
	gl_loader_glUniform1uiv = (PFNGLUNIFORM1UIVPROC)load("glUniform1uiv");
	gl_loader_glUniform2uiv = (PFNGLUNIFORM2UIVPROC)load("glUniform2uiv");
	gl_loader_glUniform3uiv = (PFNGLUNIFORM3UIVPROC)load("glUniform3uiv");
	gl_loader_glUniform4uiv = (PFNGLUNIFORM4UIVPROC)load("glUniform4uiv");
	gl_loader_glTexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)load("glTexParameterIiv");
	gl_loader_glTexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)load("glTexParameterIuiv");
	gl_loader_glGetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)load("glGetTexParameterIiv");
	gl_loader_glGetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)load("glGetTexParameterIuiv");
	gl_loader_glClearBufferiv = (PFNGLCLEARBUFFERIVPROC)load("glClearBufferiv");
	gl_loader_glClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC)load("glClearBufferuiv");
	gl_loader_glClearBufferfv = (PFNGLCLEARBUFFERFVPROC)load("glClearBufferfv");
	gl_loader_glClearBufferfi = (PFNGLCLEARBUFFERFIPROC)load("glClearBufferfi");
	gl_loader_glGetStringi = (PFNGLGETSTRINGIPROC)load("glGetStringi");
	gl_loader_glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)load("glIsRenderbuffer");
	gl_loader_glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)load("glBindRenderbuffer");
	gl_loader_glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)load("glDeleteRenderbuffers");
	gl_loader_glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)load("glGenRenderbuffers");
	gl_loader_glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)load("glRenderbufferStorage");
	gl_loader_glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)load("glGetRenderbufferParameteriv");
	gl_loader_glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC)load("glIsFramebuffer");
	gl_loader_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)load("glBindFramebuffer");
	gl_loader_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)load("glDeleteFramebuffers");
	gl_loader_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)load("glGenFramebuffers");
	gl_loader_glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)load("glCheckFramebufferStatus");
	gl_loader_glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)load("glFramebufferTexture1D");
	gl_loader_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)load("glFramebufferTexture2D");
	gl_loader_glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)load("glFramebufferTexture3D");
	gl_loader_glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)load("glFramebufferRenderbuffer");
	gl_loader_glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)load("glGetFramebufferAttachmentParameteriv");
	gl_loader_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)load("glGenerateMipmap");
	gl_loader_glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)load("glBlitFramebuffer");
	gl_loader_glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)load("glRenderbufferStorageMultisample");
	gl_loader_glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)load("glFramebufferTextureLayer");
	gl_loader_glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)load("glMapBufferRange");
	gl_loader_glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)load("glFlushMappedBufferRange");
	gl_loader_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)load("glBindVertexArray");
	gl_loader_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)load("glDeleteVertexArrays");
	gl_loader_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)load("glGenVertexArrays");
	gl_loader_glIsVertexArray = (PFNGLISVERTEXARRAYPROC)load("glIsVertexArray");
}
static void load_GL_VERSION_3_1(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_3_1) return;
	gl_loader_glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)load("glDrawArraysInstanced");
	gl_loader_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)load("glDrawElementsInstanced");
	gl_loader_glTexBuffer = (PFNGLTEXBUFFERPROC)load("glTexBuffer");
	gl_loader_glPrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC)load("glPrimitiveRestartIndex");
	gl_loader_glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)load("glCopyBufferSubData");
	gl_loader_glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)load("glGetUniformIndices");
	gl_loader_glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)load("glGetActiveUniformsiv");
	gl_loader_glGetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC)load("glGetActiveUniformName");
	gl_loader_glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)load("glGetUniformBlockIndex");
	gl_loader_glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)load("glGetActiveUniformBlockiv");
	gl_loader_glGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)load("glGetActiveUniformBlockName");
	gl_loader_glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)load("glUniformBlockBinding");
	gl_loader_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)load("glBindBufferRange");
	gl_loader_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)load("glBindBufferBase");
	gl_loader_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)load("glGetIntegeri_v");
}
static void load_GL_VERSION_3_2(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_3_2) return;
	gl_loader_glDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)load("glDrawElementsBaseVertex");
	gl_loader_glDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)load("glDrawRangeElementsBaseVertex");
	gl_loader_glDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)load("glDrawElementsInstancedBaseVertex");
	gl_loader_glMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)load("glMultiDrawElementsBaseVertex");
	gl_loader_glProvokingVertex = (PFNGLPROVOKINGVERTEXPROC)load("glProvokingVertex");
	gl_loader_glFenceSync = (PFNGLFENCESYNCPROC)load("glFenceSync");
	gl_loader_glIsSync = (PFNGLISSYNCPROC)load("glIsSync");
	gl_loader_glDeleteSync = (PFNGLDELETESYNCPROC)load("glDeleteSync");
	gl_loader_glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)load("glClientWaitSync");
	gl_loader_glWaitSync = (PFNGLWAITSYNCPROC)load("glWaitSync");
	gl_loader_glGetInteger64v = (PFNGLGETINTEGER64VPROC)load("glGetInteger64v");
	gl_loader_glGetSynciv = (PFNGLGETSYNCIVPROC)load("glGetSynciv");
	gl_loader_glGetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)load("glGetInteger64i_v");
	gl_loader_glGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)load("glGetBufferParameteri64v");
	gl_loader_glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)load("glFramebufferTexture");
	gl_loader_glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)load("glTexImage2DMultisample");
	gl_loader_glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC)load("glTexImage3DMultisample");
	gl_loader_glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)load("glGetMultisamplefv");
	gl_loader_glSampleMaski = (PFNGLSAMPLEMASKIPROC)load("glSampleMaski");
}
static void load_GL_VERSION_3_3(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_3_3) return;
	gl_loader_glBindFragDataLocationIndexed = (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)load("glBindFragDataLocationIndexed");
	gl_loader_glGetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC)load("glGetFragDataIndex");
	gl_loader_glGenSamplers = (PFNGLGENSAMPLERSPROC)load("glGenSamplers");
	gl_loader_glDeleteSamplers = (PFNGLDELETESAMPLERSPROC)load("glDeleteSamplers");
	gl_loader_glIsSampler = (PFNGLISSAMPLERPROC)load("glIsSampler");
	gl_loader_glBindSampler = (PFNGLBINDSAMPLERPROC)load("glBindSampler");
	gl_loader_glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)load("glSamplerParameteri");
	gl_loader_glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)load("glSamplerParameteriv");
	gl_loader_glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)load("glSamplerParameterf");
	gl_loader_glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)load("glSamplerParameterfv");
	gl_loader_glSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC)load("glSamplerParameterIiv");
	gl_loader_glSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC)load("glSamplerParameterIuiv");
	gl_loader_glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC)load("glGetSamplerParameteriv");
	gl_loader_glGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC)load("glGetSamplerParameterIiv");
	gl_loader_glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC)load("glGetSamplerParameterfv");
	gl_loader_glGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIVPROC)load("glGetSamplerParameterIuiv");
	gl_loader_glQueryCounter = (PFNGLQUERYCOUNTERPROC)load("glQueryCounter");
	gl_loader_glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)load("glGetQueryObjecti64v");
	gl_loader_glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)load("glGetQueryObjectui64v");
	gl_loader_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)load("glVertexAttribDivisor");
	gl_loader_glVertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC)load("glVertexAttribP1ui");
	gl_loader_glVertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC)load("glVertexAttribP1uiv");
	gl_loader_glVertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC)load("glVertexAttribP2ui");
	gl_loader_glVertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC)load("glVertexAttribP2uiv");
	gl_loader_glVertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC)load("glVertexAttribP3ui");
	gl_loader_glVertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC)load("glVertexAttribP3uiv");
	gl_loader_glVertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC)load("glVertexAttribP4ui");
	gl_loader_glVertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC)load("glVertexAttribP4uiv");
	gl_loader_glVertexP2ui = (PFNGLVERTEXP2UIPROC)load("glVertexP2ui");
	gl_loader_glVertexP2uiv = (PFNGLVERTEXP2UIVPROC)load("glVertexP2uiv");
	gl_loader_glVertexP3ui = (PFNGLVERTEXP3UIPROC)load("glVertexP3ui");
	gl_loader_glVertexP3uiv = (PFNGLVERTEXP3UIVPROC)load("glVertexP3uiv");
	gl_loader_glVertexP4ui = (PFNGLVERTEXP4UIPROC)load("glVertexP4ui");
	gl_loader_glVertexP4uiv = (PFNGLVERTEXP4UIVPROC)load("glVertexP4uiv");
	gl_loader_glTexCoordP1ui = (PFNGLTEXCOORDP1UIPROC)load("glTexCoordP1ui");
	gl_loader_glTexCoordP1uiv = (PFNGLTEXCOORDP1UIVPROC)load("glTexCoordP1uiv");
	gl_loader_glTexCoordP2ui = (PFNGLTEXCOORDP2UIPROC)load("glTexCoordP2ui");
	gl_loader_glTexCoordP2uiv = (PFNGLTEXCOORDP2UIVPROC)load("glTexCoordP2uiv");
	gl_loader_glTexCoordP3ui = (PFNGLTEXCOORDP3UIPROC)load("glTexCoordP3ui");
	gl_loader_glTexCoordP3uiv = (PFNGLTEXCOORDP3UIVPROC)load("glTexCoordP3uiv");
	gl_loader_glTexCoordP4ui = (PFNGLTEXCOORDP4UIPROC)load("glTexCoordP4ui");
	gl_loader_glTexCoordP4uiv = (PFNGLTEXCOORDP4UIVPROC)load("glTexCoordP4uiv");
	gl_loader_glMultiTexCoordP1ui = (PFNGLMULTITEXCOORDP1UIPROC)load("glMultiTexCoordP1ui");
	gl_loader_glMultiTexCoordP1uiv = (PFNGLMULTITEXCOORDP1UIVPROC)load("glMultiTexCoordP1uiv");
	gl_loader_glMultiTexCoordP2ui = (PFNGLMULTITEXCOORDP2UIPROC)load("glMultiTexCoordP2ui");
	gl_loader_glMultiTexCoordP2uiv = (PFNGLMULTITEXCOORDP2UIVPROC)load("glMultiTexCoordP2uiv");
	gl_loader_glMultiTexCoordP3ui = (PFNGLMULTITEXCOORDP3UIPROC)load("glMultiTexCoordP3ui");
	gl_loader_glMultiTexCoordP3uiv = (PFNGLMULTITEXCOORDP3UIVPROC)load("glMultiTexCoordP3uiv");
	gl_loader_glMultiTexCoordP4ui = (PFNGLMULTITEXCOORDP4UIPROC)load("glMultiTexCoordP4ui");
	gl_loader_glMultiTexCoordP4uiv = (PFNGLMULTITEXCOORDP4UIVPROC)load("glMultiTexCoordP4uiv");
	gl_loader_glNormalP3ui = (PFNGLNORMALP3UIPROC)load("glNormalP3ui");
	gl_loader_glNormalP3uiv = (PFNGLNORMALP3UIVPROC)load("glNormalP3uiv");
	gl_loader_glColorP3ui = (PFNGLCOLORP3UIPROC)load("glColorP3ui");
	gl_loader_glColorP3uiv = (PFNGLCOLORP3UIVPROC)load("glColorP3uiv");
	gl_loader_glColorP4ui = (PFNGLCOLORP4UIPROC)load("glColorP4ui");
	gl_loader_glColorP4uiv = (PFNGLCOLORP4UIVPROC)load("glColorP4uiv");
	gl_loader_glSecondaryColorP3ui = (PFNGLSECONDARYCOLORP3UIPROC)load("glSecondaryColorP3ui");
	gl_loader_glSecondaryColorP3uiv = (PFNGLSECONDARYCOLORP3UIVPROC)load("glSecondaryColorP3uiv");
}
static void load_GL_VERSION_4_0(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_4_0) return;
	gl_loader_glMinSampleShading = (PFNGLMINSAMPLESHADINGPROC)load("glMinSampleShading");
	gl_loader_glBlendEquationi = (PFNGLBLENDEQUATIONIPROC)load("glBlendEquationi");
	gl_loader_glBlendEquationSeparatei = (PFNGLBLENDEQUATIONSEPARATEIPROC)load("glBlendEquationSeparatei");
	gl_loader_glBlendFunci = (PFNGLBLENDFUNCIPROC)load("glBlendFunci");
	gl_loader_glBlendFuncSeparatei = (PFNGLBLENDFUNCSEPARATEIPROC)load("glBlendFuncSeparatei");
	gl_loader_glDrawArraysIndirect = (PFNGLDRAWARRAYSINDIRECTPROC)load("glDrawArraysIndirect");
	gl_loader_glDrawElementsIndirect = (PFNGLDRAWELEMENTSINDIRECTPROC)load("glDrawElementsIndirect");
	gl_loader_glUniform1d = (PFNGLUNIFORM1DPROC)load("glUniform1d");
	gl_loader_glUniform2d = (PFNGLUNIFORM2DPROC)load("glUniform2d");
	gl_loader_glUniform3d = (PFNGLUNIFORM3DPROC)load("glUniform3d");
	gl_loader_glUniform4d = (PFNGLUNIFORM4DPROC)load("glUniform4d");
	gl_loader_glUniform1dv = (PFNGLUNIFORM1DVPROC)load("glUniform1dv");
	gl_loader_glUniform2dv = (PFNGLUNIFORM2DVPROC)load("glUniform2dv");
	gl_loader_glUniform3dv = (PFNGLUNIFORM3DVPROC)load("glUniform3dv");
	gl_loader_glUniform4dv = (PFNGLUNIFORM4DVPROC)load("glUniform4dv");
	gl_loader_glUniformMatrix2dv = (PFNGLUNIFORMMATRIX2DVPROC)load("glUniformMatrix2dv");
	gl_loader_glUniformMatrix3dv = (PFNGLUNIFORMMATRIX3DVPROC)load("glUniformMatrix3dv");
	gl_loader_glUniformMatrix4dv = (PFNGLUNIFORMMATRIX4DVPROC)load("glUniformMatrix4dv");
	gl_loader_glUniformMatrix2x3dv = (PFNGLUNIFORMMATRIX2X3DVPROC)load("glUniformMatrix2x3dv");
	gl_loader_glUniformMatrix2x4dv = (PFNGLUNIFORMMATRIX2X4DVPROC)load("glUniformMatrix2x4dv");
	gl_loader_glUniformMatrix3x2dv = (PFNGLUNIFORMMATRIX3X2DVPROC)load("glUniformMatrix3x2dv");
	gl_loader_glUniformMatrix3x4dv = (PFNGLUNIFORMMATRIX3X4DVPROC)load("glUniformMatrix3x4dv");
	gl_loader_glUniformMatrix4x2dv = (PFNGLUNIFORMMATRIX4X2DVPROC)load("glUniformMatrix4x2dv");
	gl_loader_glUniformMatrix4x3dv = (PFNGLUNIFORMMATRIX4X3DVPROC)load("glUniformMatrix4x3dv");
	gl_loader_glGetUniformdv = (PFNGLGETUNIFORMDVPROC)load("glGetUniformdv");
	gl_loader_glGetSubroutineUniformLocation = (PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC)load("glGetSubroutineUniformLocation");
	gl_loader_glGetSubroutineIndex = (PFNGLGETSUBROUTINEINDEXPROC)load("glGetSubroutineIndex");
	gl_loader_glGetActiveSubroutineUniformiv = (PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC)load("glGetActiveSubroutineUniformiv");
	gl_loader_glGetActiveSubroutineUniformName = (PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC)load("glGetActiveSubroutineUniformName");
	gl_loader_glGetActiveSubroutineName = (PFNGLGETACTIVESUBROUTINENAMEPROC)load("glGetActiveSubroutineName");
	gl_loader_glUniformSubroutinesuiv = (PFNGLUNIFORMSUBROUTINESUIVPROC)load("glUniformSubroutinesuiv");
	gl_loader_glGetUniformSubroutineuiv = (PFNGLGETUNIFORMSUBROUTINEUIVPROC)load("glGetUniformSubroutineuiv");
	gl_loader_glGetProgramStageiv = (PFNGLGETPROGRAMSTAGEIVPROC)load("glGetProgramStageiv");
	gl_loader_glPatchParameteri = (PFNGLPATCHPARAMETERIPROC)load("glPatchParameteri");
	gl_loader_glPatchParameterfv = (PFNGLPATCHPARAMETERFVPROC)load("glPatchParameterfv");
	gl_loader_glBindTransformFeedback = (PFNGLBINDTRANSFORMFEEDBACKPROC)load("glBindTransformFeedback");
	gl_loader_glDeleteTransformFeedbacks = (PFNGLDELETETRANSFORMFEEDBACKSPROC)load("glDeleteTransformFeedbacks");
	gl_loader_glGenTransformFeedbacks = (PFNGLGENTRANSFORMFEEDBACKSPROC)load("glGenTransformFeedbacks");
	gl_loader_glIsTransformFeedback = (PFNGLISTRANSFORMFEEDBACKPROC)load("glIsTransformFeedback");
	gl_loader_glPauseTransformFeedback = (PFNGLPAUSETRANSFORMFEEDBACKPROC)load("glPauseTransformFeedback");
	gl_loader_glResumeTransformFeedback = (PFNGLRESUMETRANSFORMFEEDBACKPROC)load("glResumeTransformFeedback");
	gl_loader_glDrawTransformFeedback = (PFNGLDRAWTRANSFORMFEEDBACKPROC)load("glDrawTransformFeedback");
	gl_loader_glDrawTransformFeedbackStream = (PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC)load("glDrawTransformFeedbackStream");
	gl_loader_glBeginQueryIndexed = (PFNGLBEGINQUERYINDEXEDPROC)load("glBeginQueryIndexed");
	gl_loader_glEndQueryIndexed = (PFNGLENDQUERYINDEXEDPROC)load("glEndQueryIndexed");
	gl_loader_glGetQueryIndexediv = (PFNGLGETQUERYINDEXEDIVPROC)load("glGetQueryIndexediv");
}
static void load_GL_VERSION_4_1(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_4_1) return;
	gl_loader_glReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC)load("glReleaseShaderCompiler");
	gl_loader_glShaderBinary = (PFNGLSHADERBINARYPROC)load("glShaderBinary");
	gl_loader_glGetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC)load("glGetShaderPrecisionFormat");
	gl_loader_glDepthRangef = (PFNGLDEPTHRANGEFPROC)load("glDepthRangef");
	gl_loader_glClearDepthf = (PFNGLCLEARDEPTHFPROC)load("glClearDepthf");
	gl_loader_glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)load("glGetProgramBinary");
	gl_loader_glProgramBinary = (PFNGLPROGRAMBINARYPROC)load("glProgramBinary");
	gl_loader_glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)load("glProgramParameteri");
	gl_loader_glUseProgramStages = (PFNGLUSEPROGRAMSTAGESPROC)load("glUseProgramStages");
	gl_loader_glActiveShaderProgram = (PFNGLACTIVESHADERPROGRAMPROC)load("glActiveShaderProgram");
	gl_loader_glCreateShaderProgramv = (PFNGLCREATESHADERPROGRAMVPROC)load("glCreateShaderProgramv");
	gl_loader_glBindProgramPipeline = (PFNGLBINDPROGRAMPIPELINEPROC)load("glBindProgramPipeline");
	gl_loader_glDeleteProgramPipelines = (PFNGLDELETEPROGRAMPIPELINESPROC)load("glDeleteProgramPipelines");
	gl_loader_glGenProgramPipelines = (PFNGLGENPROGRAMPIPELINESPROC)load("glGenProgramPipelines");
	gl_loader_glIsProgramPipeline = (PFNGLISPROGRAMPIPELINEPROC)load("glIsProgramPipeline");
	gl_loader_glGetProgramPipelineiv = (PFNGLGETPROGRAMPIPELINEIVPROC)load("glGetProgramPipelineiv");
	gl_loader_glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)load("glProgramParameteri");
	gl_loader_glProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)load("glProgramUniform1i");
	gl_loader_glProgramUniform1iv = (PFNGLPROGRAMUNIFORM1IVPROC)load("glProgramUniform1iv");
	gl_loader_glProgramUniform1f = (PFNGLPROGRAMUNIFORM1FPROC)load("glProgramUniform1f");
	gl_loader_glProgramUniform1fv = (PFNGLPROGRAMUNIFORM1FVPROC)load("glProgramUniform1fv");
	gl_loader_glProgramUniform1d = (PFNGLPROGRAMUNIFORM1DPROC)load("glProgramUniform1d");
	gl_loader_glProgramUniform1dv = (PFNGLPROGRAMUNIFORM1DVPROC)load("glProgramUniform1dv");
	gl_loader_glProgramUniform1ui = (PFNGLPROGRAMUNIFORM1UIPROC)load("glProgramUniform1ui");
	gl_loader_glProgramUniform1uiv = (PFNGLPROGRAMUNIFORM1UIVPROC)load("glProgramUniform1uiv");
	gl_loader_glProgramUniform2i = (PFNGLPROGRAMUNIFORM2IPROC)load("glProgramUniform2i");
	gl_loader_glProgramUniform2iv = (PFNGLPROGRAMUNIFORM2IVPROC)load("glProgramUniform2iv");
	gl_loader_glProgramUniform2f = (PFNGLPROGRAMUNIFORM2FPROC)load("glProgramUniform2f");
	gl_loader_glProgramUniform2fv = (PFNGLPROGRAMUNIFORM2FVPROC)load("glProgramUniform2fv");
	gl_loader_glProgramUniform2d = (PFNGLPROGRAMUNIFORM2DPROC)load("glProgramUniform2d");
	gl_loader_glProgramUniform2dv = (PFNGLPROGRAMUNIFORM2DVPROC)load("glProgramUniform2dv");
	gl_loader_glProgramUniform2ui = (PFNGLPROGRAMUNIFORM2UIPROC)load("glProgramUniform2ui");
	gl_loader_glProgramUniform2uiv = (PFNGLPROGRAMUNIFORM2UIVPROC)load("glProgramUniform2uiv");
	gl_loader_glProgramUniform3i = (PFNGLPROGRAMUNIFORM3IPROC)load("glProgramUniform3i");
	gl_loader_glProgramUniform3iv = (PFNGLPROGRAMUNIFORM3IVPROC)load("glProgramUniform3iv");
	gl_loader_glProgramUniform3f = (PFNGLPROGRAMUNIFORM3FPROC)load("glProgramUniform3f");
	gl_loader_glProgramUniform3fv = (PFNGLPROGRAMUNIFORM3FVPROC)load("glProgramUniform3fv");
	gl_loader_glProgramUniform3d = (PFNGLPROGRAMUNIFORM3DPROC)load("glProgramUniform3d");
	gl_loader_glProgramUniform3dv = (PFNGLPROGRAMUNIFORM3DVPROC)load("glProgramUniform3dv");
	gl_loader_glProgramUniform3ui = (PFNGLPROGRAMUNIFORM3UIPROC)load("glProgramUniform3ui");
	gl_loader_glProgramUniform3uiv = (PFNGLPROGRAMUNIFORM3UIVPROC)load("glProgramUniform3uiv");
	gl_loader_glProgramUniform4i = (PFNGLPROGRAMUNIFORM4IPROC)load("glProgramUniform4i");
	gl_loader_glProgramUniform4iv = (PFNGLPROGRAMUNIFORM4IVPROC)load("glProgramUniform4iv");
	gl_loader_glProgramUniform4f = (PFNGLPROGRAMUNIFORM4FPROC)load("glProgramUniform4f");
	gl_loader_glProgramUniform4fv = (PFNGLPROGRAMUNIFORM4FVPROC)load("glProgramUniform4fv");
	gl_loader_glProgramUniform4d = (PFNGLPROGRAMUNIFORM4DPROC)load("glProgramUniform4d");
	gl_loader_glProgramUniform4dv = (PFNGLPROGRAMUNIFORM4DVPROC)load("glProgramUniform4dv");
	gl_loader_glProgramUniform4ui = (PFNGLPROGRAMUNIFORM4UIPROC)load("glProgramUniform4ui");
	gl_loader_glProgramUniform4uiv = (PFNGLPROGRAMUNIFORM4UIVPROC)load("glProgramUniform4uiv");
	gl_loader_glProgramUniformMatrix2fv = (PFNGLPROGRAMUNIFORMMATRIX2FVPROC)load("glProgramUniformMatrix2fv");
	gl_loader_glProgramUniformMatrix3fv = (PFNGLPROGRAMUNIFORMMATRIX3FVPROC)load("glProgramUniformMatrix3fv");
	gl_loader_glProgramUniformMatrix4fv = (PFNGLPROGRAMUNIFORMMATRIX4FVPROC)load("glProgramUniformMatrix4fv");
	gl_loader_glProgramUniformMatrix2dv = (PFNGLPROGRAMUNIFORMMATRIX2DVPROC)load("glProgramUniformMatrix2dv");
	gl_loader_glProgramUniformMatrix3dv = (PFNGLPROGRAMUNIFORMMATRIX3DVPROC)load("glProgramUniformMatrix3dv");
	gl_loader_glProgramUniformMatrix4dv = (PFNGLPROGRAMUNIFORMMATRIX4DVPROC)load("glProgramUniformMatrix4dv");
	gl_loader_glProgramUniformMatrix2x3fv = (PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC)load("glProgramUniformMatrix2x3fv");
	gl_loader_glProgramUniformMatrix3x2fv = (PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC)load("glProgramUniformMatrix3x2fv");
	gl_loader_glProgramUniformMatrix2x4fv = (PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC)load("glProgramUniformMatrix2x4fv");
	gl_loader_glProgramUniformMatrix4x2fv = (PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC)load("glProgramUniformMatrix4x2fv");
	gl_loader_glProgramUniformMatrix3x4fv = (PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC)load("glProgramUniformMatrix3x4fv");
	gl_loader_glProgramUniformMatrix4x3fv = (PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC)load("glProgramUniformMatrix4x3fv");
	gl_loader_glProgramUniformMatrix2x3dv = (PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC)load("glProgramUniformMatrix2x3dv");
	gl_loader_glProgramUniformMatrix3x2dv = (PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC)load("glProgramUniformMatrix3x2dv");
	gl_loader_glProgramUniformMatrix2x4dv = (PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC)load("glProgramUniformMatrix2x4dv");
	gl_loader_glProgramUniformMatrix4x2dv = (PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC)load("glProgramUniformMatrix4x2dv");
	gl_loader_glProgramUniformMatrix3x4dv = (PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC)load("glProgramUniformMatrix3x4dv");
	gl_loader_glProgramUniformMatrix4x3dv = (PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC)load("glProgramUniformMatrix4x3dv");
	gl_loader_glValidateProgramPipeline = (PFNGLVALIDATEPROGRAMPIPELINEPROC)load("glValidateProgramPipeline");
	gl_loader_glGetProgramPipelineInfoLog = (PFNGLGETPROGRAMPIPELINEINFOLOGPROC)load("glGetProgramPipelineInfoLog");
	gl_loader_glVertexAttribL1d = (PFNGLVERTEXATTRIBL1DPROC)load("glVertexAttribL1d");
	gl_loader_glVertexAttribL2d = (PFNGLVERTEXATTRIBL2DPROC)load("glVertexAttribL2d");
	gl_loader_glVertexAttribL3d = (PFNGLVERTEXATTRIBL3DPROC)load("glVertexAttribL3d");
	gl_loader_glVertexAttribL4d = (PFNGLVERTEXATTRIBL4DPROC)load("glVertexAttribL4d");
	gl_loader_glVertexAttribL1dv = (PFNGLVERTEXATTRIBL1DVPROC)load("glVertexAttribL1dv");
	gl_loader_glVertexAttribL2dv = (PFNGLVERTEXATTRIBL2DVPROC)load("glVertexAttribL2dv");
	gl_loader_glVertexAttribL3dv = (PFNGLVERTEXATTRIBL3DVPROC)load("glVertexAttribL3dv");
	gl_loader_glVertexAttribL4dv = (PFNGLVERTEXATTRIBL4DVPROC)load("glVertexAttribL4dv");
	gl_loader_glVertexAttribLPointer = (PFNGLVERTEXATTRIBLPOINTERPROC)load("glVertexAttribLPointer");
	gl_loader_glGetVertexAttribLdv = (PFNGLGETVERTEXATTRIBLDVPROC)load("glGetVertexAttribLdv");
	gl_loader_glViewportArrayv = (PFNGLVIEWPORTARRAYVPROC)load("glViewportArrayv");
	gl_loader_glViewportIndexedf = (PFNGLVIEWPORTINDEXEDFPROC)load("glViewportIndexedf");
	gl_loader_glViewportIndexedfv = (PFNGLVIEWPORTINDEXEDFVPROC)load("glViewportIndexedfv");
	gl_loader_glScissorArrayv = (PFNGLSCISSORARRAYVPROC)load("glScissorArrayv");
	gl_loader_glScissorIndexed = (PFNGLSCISSORINDEXEDPROC)load("glScissorIndexed");
	gl_loader_glScissorIndexedv = (PFNGLSCISSORINDEXEDVPROC)load("glScissorIndexedv");
	gl_loader_glDepthRangeArrayv = (PFNGLDEPTHRANGEARRAYVPROC)load("glDepthRangeArrayv");
	gl_loader_glDepthRangeIndexed = (PFNGLDEPTHRANGEINDEXEDPROC)load("glDepthRangeIndexed");
	gl_loader_glGetFloati_v = (PFNGLGETFLOATI_VPROC)load("glGetFloati_v");
	gl_loader_glGetDoublei_v = (PFNGLGETDOUBLEI_VPROC)load("glGetDoublei_v");
}
static void load_GL_VERSION_4_2(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_4_2) return;
	gl_loader_glDrawArraysInstancedBaseInstance = (PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)load("glDrawArraysInstancedBaseInstance");
	gl_loader_glDrawElementsInstancedBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC)load("glDrawElementsInstancedBaseInstance");
	gl_loader_glDrawElementsInstancedBaseVertexBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC)load("glDrawElementsInstancedBaseVertexBaseInstance");
	gl_loader_glGetInternalformativ = (PFNGLGETINTERNALFORMATIVPROC)load("glGetInternalformativ");
	gl_loader_glGetActiveAtomicCounterBufferiv = (PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC)load("glGetActiveAtomicCounterBufferiv");
	gl_loader_glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)load("glBindImageTexture");
	gl_loader_glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)load("glMemoryBarrier");
	gl_loader_glTexStorage1D = (PFNGLTEXSTORAGE1DPROC)load("glTexStorage1D");
	gl_loader_glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)load("glTexStorage2D");
	gl_loader_glTexStorage3D = (PFNGLTEXSTORAGE3DPROC)load("glTexStorage3D");
	gl_loader_glDrawTransformFeedbackInstanced = (PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC)load("glDrawTransformFeedbackInstanced");
	gl_loader_glDrawTransformFeedbackStreamInstanced = (PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC)load("glDrawTransformFeedbackStreamInstanced");
}
static void load_GL_VERSION_4_3(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_4_3) return;
	gl_loader_glClearBufferData = (PFNGLCLEARBUFFERDATAPROC)load("glClearBufferData");
	gl_loader_glClearBufferSubData = (PFNGLCLEARBUFFERSUBDATAPROC)load("glClearBufferSubData");
	gl_loader_glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)load("glDispatchCompute");
	gl_loader_glDispatchComputeIndirect = (PFNGLDISPATCHCOMPUTEINDIRECTPROC)load("glDispatchComputeIndirect");
	gl_loader_glCopyImageSubData = (PFNGLCOPYIMAGESUBDATAPROC)load("glCopyImageSubData");
	gl_loader_glFramebufferParameteri = (PFNGLFRAMEBUFFERPARAMETERIPROC)load("glFramebufferParameteri");
	gl_loader_glGetFramebufferParameteriv = (PFNGLGETFRAMEBUFFERPARAMETERIVPROC)load("glGetFramebufferParameteriv");
	gl_loader_glGetInternalformati64v = (PFNGLGETINTERNALFORMATI64VPROC)load("glGetInternalformati64v");
	gl_loader_glInvalidateTexSubImage = (PFNGLINVALIDATETEXSUBIMAGEPROC)load("glInvalidateTexSubImage");
	gl_loader_glInvalidateTexImage = (PFNGLINVALIDATETEXIMAGEPROC)load("glInvalidateTexImage");
	gl_loader_glInvalidateBufferSubData = (PFNGLINVALIDATEBUFFERSUBDATAPROC)load("glInvalidateBufferSubData");
	gl_loader_glInvalidateBufferData = (PFNGLINVALIDATEBUFFERDATAPROC)load("glInvalidateBufferData");
	gl_loader_glInvalidateFramebuffer = (PFNGLINVALIDATEFRAMEBUFFERPROC)load("glInvalidateFramebuffer");
	gl_loader_glInvalidateSubFramebuffer = (PFNGLINVALIDATESUBFRAMEBUFFERPROC)load("glInvalidateSubFramebuffer");
	gl_loader_glMultiDrawArraysIndirect = (PFNGLMULTIDRAWARRAYSINDIRECTPROC)load("glMultiDrawArraysIndirect");
	gl_loader_glMultiDrawElementsIndirect = (PFNGLMULTIDRAWELEMENTSINDIRECTPROC)load("glMultiDrawElementsIndirect");
	gl_loader_glGetProgramInterfaceiv = (PFNGLGETPROGRAMINTERFACEIVPROC)load("glGetProgramInterfaceiv");
	gl_loader_glGetProgramResourceIndex = (PFNGLGETPROGRAMRESOURCEINDEXPROC)load("glGetProgramResourceIndex");
	gl_loader_glGetProgramResourceName = (PFNGLGETPROGRAMRESOURCENAMEPROC)load("glGetProgramResourceName");
	gl_loader_glGetProgramResourceiv = (PFNGLGETPROGRAMRESOURCEIVPROC)load("glGetProgramResourceiv");
	gl_loader_glGetProgramResourceLocation = (PFNGLGETPROGRAMRESOURCELOCATIONPROC)load("glGetProgramResourceLocation");
	gl_loader_glGetProgramResourceLocationIndex = (PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC)load("glGetProgramResourceLocationIndex");
	gl_loader_glShaderStorageBlockBinding = (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)load("glShaderStorageBlockBinding");
	gl_loader_glTexBufferRange = (PFNGLTEXBUFFERRANGEPROC)load("glTexBufferRange");
	gl_loader_glTexStorage2DMultisample = (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)load("glTexStorage2DMultisample");
	gl_loader_glTexStorage3DMultisample = (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)load("glTexStorage3DMultisample");
	gl_loader_glTextureView = (PFNGLTEXTUREVIEWPROC)load("glTextureView");
	gl_loader_glBindVertexBuffer = (PFNGLBINDVERTEXBUFFERPROC)load("glBindVertexBuffer");
	gl_loader_glVertexAttribFormat = (PFNGLVERTEXATTRIBFORMATPROC)load("glVertexAttribFormat");
	gl_loader_glVertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMATPROC)load("glVertexAttribIFormat");
	gl_loader_glVertexAttribLFormat = (PFNGLVERTEXATTRIBLFORMATPROC)load("glVertexAttribLFormat");
	gl_loader_glVertexAttribBinding = (PFNGLVERTEXATTRIBBINDINGPROC)load("glVertexAttribBinding");
	gl_loader_glVertexBindingDivisor = (PFNGLVERTEXBINDINGDIVISORPROC)load("glVertexBindingDivisor");
	gl_loader_glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)load("glDebugMessageControl");
	gl_loader_glDebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC)load("glDebugMessageInsert");
	gl_loader_glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)load("glDebugMessageCallback");
	gl_loader_glGetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC)load("glGetDebugMessageLog");
	gl_loader_glPushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC)load("glPushDebugGroup");
	gl_loader_glPopDebugGroup = (PFNGLPOPDEBUGGROUPPROC)load("glPopDebugGroup");
	gl_loader_glObjectLabel = (PFNGLOBJECTLABELPROC)load("glObjectLabel");
	gl_loader_glGetObjectLabel = (PFNGLGETOBJECTLABELPROC)load("glGetObjectLabel");
	gl_loader_glObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC)load("glObjectPtrLabel");
	gl_loader_glGetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC)load("glGetObjectPtrLabel");
	gl_loader_glGetPointerv = (PFNGLGETPOINTERVPROC)load("glGetPointerv");
}
static void load_GL_VERSION_4_4(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_4_4) return;
	gl_loader_glBufferStorage = (PFNGLBUFFERSTORAGEPROC)load("glBufferStorage");
	gl_loader_glClearTexImage = (PFNGLCLEARTEXIMAGEPROC)load("glClearTexImage");
	gl_loader_glClearTexSubImage = (PFNGLCLEARTEXSUBIMAGEPROC)load("glClearTexSubImage");
	gl_loader_glBindBuffersBase = (PFNGLBINDBUFFERSBASEPROC)load("glBindBuffersBase");
	gl_loader_glBindBuffersRange = (PFNGLBINDBUFFERSRANGEPROC)load("glBindBuffersRange");
	gl_loader_glBindTextures = (PFNGLBINDTEXTURESPROC)load("glBindTextures");
	gl_loader_glBindSamplers = (PFNGLBINDSAMPLERSPROC)load("glBindSamplers");
	gl_loader_glBindImageTextures = (PFNGLBINDIMAGETEXTURESPROC)load("glBindImageTextures");
	gl_loader_glBindVertexBuffers = (PFNGLBINDVERTEXBUFFERSPROC)load("glBindVertexBuffers");
}
static void load_GL_VERSION_4_5(GLLOADERloadproc load) {
	if(!GLLOADER_GL_VERSION_4_5) return;
	gl_loader_glClipControl = (PFNGLCLIPCONTROLPROC)load("glClipControl");
	gl_loader_glCreateTransformFeedbacks = (PFNGLCREATETRANSFORMFEEDBACKSPROC)load("glCreateTransformFeedbacks");
	gl_loader_glTransformFeedbackBufferBase = (PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC)load("glTransformFeedbackBufferBase");
	gl_loader_glTransformFeedbackBufferRange = (PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC)load("glTransformFeedbackBufferRange");
	gl_loader_glGetTransformFeedbackiv = (PFNGLGETTRANSFORMFEEDBACKIVPROC)load("glGetTransformFeedbackiv");
	gl_loader_glGetTransformFeedbacki_v = (PFNGLGETTRANSFORMFEEDBACKI_VPROC)load("glGetTransformFeedbacki_v");
	gl_loader_glGetTransformFeedbacki64_v = (PFNGLGETTRANSFORMFEEDBACKI64_VPROC)load("glGetTransformFeedbacki64_v");
	gl_loader_glCreateBuffers = (PFNGLCREATEBUFFERSPROC)load("glCreateBuffers");
	gl_loader_glNamedBufferStorage = (PFNGLNAMEDBUFFERSTORAGEPROC)load("glNamedBufferStorage");
	gl_loader_glNamedBufferData = (PFNGLNAMEDBUFFERDATAPROC)load("glNamedBufferData");
	gl_loader_glNamedBufferSubData = (PFNGLNAMEDBUFFERSUBDATAPROC)load("glNamedBufferSubData");
	gl_loader_glCopyNamedBufferSubData = (PFNGLCOPYNAMEDBUFFERSUBDATAPROC)load("glCopyNamedBufferSubData");
	gl_loader_glClearNamedBufferData = (PFNGLCLEARNAMEDBUFFERDATAPROC)load("glClearNamedBufferData");
	gl_loader_glClearNamedBufferSubData = (PFNGLCLEARNAMEDBUFFERSUBDATAPROC)load("glClearNamedBufferSubData");
	gl_loader_glMapNamedBuffer = (PFNGLMAPNAMEDBUFFERPROC)load("glMapNamedBuffer");
	gl_loader_glMapNamedBufferRange = (PFNGLMAPNAMEDBUFFERRANGEPROC)load("glMapNamedBufferRange");
	gl_loader_glUnmapNamedBuffer = (PFNGLUNMAPNAMEDBUFFERPROC)load("glUnmapNamedBuffer");
	gl_loader_glFlushMappedNamedBufferRange = (PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC)load("glFlushMappedNamedBufferRange");
	gl_loader_glGetNamedBufferParameteriv = (PFNGLGETNAMEDBUFFERPARAMETERIVPROC)load("glGetNamedBufferParameteriv");
	gl_loader_glGetNamedBufferParameteri64v = (PFNGLGETNAMEDBUFFERPARAMETERI64VPROC)load("glGetNamedBufferParameteri64v");
	gl_loader_glGetNamedBufferPointerv = (PFNGLGETNAMEDBUFFERPOINTERVPROC)load("glGetNamedBufferPointerv");
	gl_loader_glGetNamedBufferSubData = (PFNGLGETNAMEDBUFFERSUBDATAPROC)load("glGetNamedBufferSubData");
	gl_loader_glCreateFramebuffers = (PFNGLCREATEFRAMEBUFFERSPROC)load("glCreateFramebuffers");
	gl_loader_glNamedFramebufferRenderbuffer = (PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC)load("glNamedFramebufferRenderbuffer");
	gl_loader_glNamedFramebufferParameteri = (PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC)load("glNamedFramebufferParameteri");
	gl_loader_glNamedFramebufferTexture = (PFNGLNAMEDFRAMEBUFFERTEXTUREPROC)load("glNamedFramebufferTexture");
	gl_loader_glNamedFramebufferTextureLayer = (PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC)load("glNamedFramebufferTextureLayer");
	gl_loader_glNamedFramebufferDrawBuffer = (PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC)load("glNamedFramebufferDrawBuffer");
	gl_loader_glNamedFramebufferDrawBuffers = (PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC)load("glNamedFramebufferDrawBuffers");
	gl_loader_glNamedFramebufferReadBuffer = (PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC)load("glNamedFramebufferReadBuffer");
	gl_loader_glInvalidateNamedFramebufferData = (PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC)load("glInvalidateNamedFramebufferData");
	gl_loader_glInvalidateNamedFramebufferSubData = (PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC)load("glInvalidateNamedFramebufferSubData");
	gl_loader_glClearNamedFramebufferiv = (PFNGLCLEARNAMEDFRAMEBUFFERIVPROC)load("glClearNamedFramebufferiv");
	gl_loader_glClearNamedFramebufferuiv = (PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC)load("glClearNamedFramebufferuiv");
	gl_loader_glClearNamedFramebufferfv = (PFNGLCLEARNAMEDFRAMEBUFFERFVPROC)load("glClearNamedFramebufferfv");
	gl_loader_glClearNamedFramebufferfi = (PFNGLCLEARNAMEDFRAMEBUFFERFIPROC)load("glClearNamedFramebufferfi");
	gl_loader_glBlitNamedFramebuffer = (PFNGLBLITNAMEDFRAMEBUFFERPROC)load("glBlitNamedFramebuffer");
	gl_loader_glCheckNamedFramebufferStatus = (PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC)load("glCheckNamedFramebufferStatus");
	gl_loader_glGetNamedFramebufferParameteriv = (PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC)load("glGetNamedFramebufferParameteriv");
	gl_loader_glGetNamedFramebufferAttachmentParameteriv = (PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC)load("glGetNamedFramebufferAttachmentParameteriv");
	gl_loader_glCreateRenderbuffers = (PFNGLCREATERENDERBUFFERSPROC)load("glCreateRenderbuffers");
	gl_loader_glNamedRenderbufferStorage = (PFNGLNAMEDRENDERBUFFERSTORAGEPROC)load("glNamedRenderbufferStorage");
	gl_loader_glNamedRenderbufferStorageMultisample = (PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC)load("glNamedRenderbufferStorageMultisample");
	gl_loader_glGetNamedRenderbufferParameteriv = (PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC)load("glGetNamedRenderbufferParameteriv");
	gl_loader_glCreateTextures = (PFNGLCREATETEXTURESPROC)load("glCreateTextures");
	gl_loader_glTextureBuffer = (PFNGLTEXTUREBUFFERPROC)load("glTextureBuffer");
	gl_loader_glTextureBufferRange = (PFNGLTEXTUREBUFFERRANGEPROC)load("glTextureBufferRange");
	gl_loader_glTextureStorage1D = (PFNGLTEXTURESTORAGE1DPROC)load("glTextureStorage1D");
	gl_loader_glTextureStorage2D = (PFNGLTEXTURESTORAGE2DPROC)load("glTextureStorage2D");
	gl_loader_glTextureStorage3D = (PFNGLTEXTURESTORAGE3DPROC)load("glTextureStorage3D");
	gl_loader_glTextureStorage2DMultisample = (PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC)load("glTextureStorage2DMultisample");
	gl_loader_glTextureStorage3DMultisample = (PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC)load("glTextureStorage3DMultisample");
	gl_loader_glTextureSubImage1D = (PFNGLTEXTURESUBIMAGE1DPROC)load("glTextureSubImage1D");
	gl_loader_glTextureSubImage2D = (PFNGLTEXTURESUBIMAGE2DPROC)load("glTextureSubImage2D");
	gl_loader_glTextureSubImage3D = (PFNGLTEXTURESUBIMAGE3DPROC)load("glTextureSubImage3D");
	gl_loader_glCompressedTextureSubImage1D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC)load("glCompressedTextureSubImage1D");
	gl_loader_glCompressedTextureSubImage2D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC)load("glCompressedTextureSubImage2D");
	gl_loader_glCompressedTextureSubImage3D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC)load("glCompressedTextureSubImage3D");
	gl_loader_glCopyTextureSubImage1D = (PFNGLCOPYTEXTURESUBIMAGE1DPROC)load("glCopyTextureSubImage1D");
	gl_loader_glCopyTextureSubImage2D = (PFNGLCOPYTEXTURESUBIMAGE2DPROC)load("glCopyTextureSubImage2D");
	gl_loader_glCopyTextureSubImage3D = (PFNGLCOPYTEXTURESUBIMAGE3DPROC)load("glCopyTextureSubImage3D");
	gl_loader_glTextureParameterf = (PFNGLTEXTUREPARAMETERFPROC)load("glTextureParameterf");
	gl_loader_glTextureParameterfv = (PFNGLTEXTUREPARAMETERFVPROC)load("glTextureParameterfv");
	gl_loader_glTextureParameteri = (PFNGLTEXTUREPARAMETERIPROC)load("glTextureParameteri");
	gl_loader_glTextureParameterIiv = (PFNGLTEXTUREPARAMETERIIVPROC)load("glTextureParameterIiv");
	gl_loader_glTextureParameterIuiv = (PFNGLTEXTUREPARAMETERIUIVPROC)load("glTextureParameterIuiv");
	gl_loader_glTextureParameteriv = (PFNGLTEXTUREPARAMETERIVPROC)load("glTextureParameteriv");
	gl_loader_glGenerateTextureMipmap = (PFNGLGENERATETEXTUREMIPMAPPROC)load("glGenerateTextureMipmap");
	gl_loader_glBindTextureUnit = (PFNGLBINDTEXTUREUNITPROC)load("glBindTextureUnit");
	gl_loader_glGetTextureImage = (PFNGLGETTEXTUREIMAGEPROC)load("glGetTextureImage");
	gl_loader_glGetCompressedTextureImage = (PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC)load("glGetCompressedTextureImage");
	gl_loader_glGetTextureLevelParameterfv = (PFNGLGETTEXTURELEVELPARAMETERFVPROC)load("glGetTextureLevelParameterfv");
	gl_loader_glGetTextureLevelParameteriv = (PFNGLGETTEXTURELEVELPARAMETERIVPROC)load("glGetTextureLevelParameteriv");
	gl_loader_glGetTextureParameterfv = (PFNGLGETTEXTUREPARAMETERFVPROC)load("glGetTextureParameterfv");
	gl_loader_glGetTextureParameterIiv = (PFNGLGETTEXTUREPARAMETERIIVPROC)load("glGetTextureParameterIiv");
	gl_loader_glGetTextureParameterIuiv = (PFNGLGETTEXTUREPARAMETERIUIVPROC)load("glGetTextureParameterIuiv");
	gl_loader_glGetTextureParameteriv = (PFNGLGETTEXTUREPARAMETERIVPROC)load("glGetTextureParameteriv");
	gl_loader_glCreateVertexArrays = (PFNGLCREATEVERTEXARRAYSPROC)load("glCreateVertexArrays");
	gl_loader_glDisableVertexArrayAttrib = (PFNGLDISABLEVERTEXARRAYATTRIBPROC)load("glDisableVertexArrayAttrib");
	gl_loader_glEnableVertexArrayAttrib = (PFNGLENABLEVERTEXARRAYATTRIBPROC)load("glEnableVertexArrayAttrib");
	gl_loader_glVertexArrayElementBuffer = (PFNGLVERTEXARRAYELEMENTBUFFERPROC)load("glVertexArrayElementBuffer");
	gl_loader_glVertexArrayVertexBuffer = (PFNGLVERTEXARRAYVERTEXBUFFERPROC)load("glVertexArrayVertexBuffer");
	gl_loader_glVertexArrayVertexBuffers = (PFNGLVERTEXARRAYVERTEXBUFFERSPROC)load("glVertexArrayVertexBuffers");
	gl_loader_glVertexArrayAttribBinding = (PFNGLVERTEXARRAYATTRIBBINDINGPROC)load("glVertexArrayAttribBinding");
	gl_loader_glVertexArrayAttribFormat = (PFNGLVERTEXARRAYATTRIBFORMATPROC)load("glVertexArrayAttribFormat");
	gl_loader_glVertexArrayAttribIFormat = (PFNGLVERTEXARRAYATTRIBIFORMATPROC)load("glVertexArrayAttribIFormat");
	gl_loader_glVertexArrayAttribLFormat = (PFNGLVERTEXARRAYATTRIBLFORMATPROC)load("glVertexArrayAttribLFormat");
	gl_loader_glVertexArrayBindingDivisor = (PFNGLVERTEXARRAYBINDINGDIVISORPROC)load("glVertexArrayBindingDivisor");
	gl_loader_glGetVertexArrayiv = (PFNGLGETVERTEXARRAYIVPROC)load("glGetVertexArrayiv");
	gl_loader_glGetVertexArrayIndexediv = (PFNGLGETVERTEXARRAYINDEXEDIVPROC)load("glGetVertexArrayIndexediv");
	gl_loader_glGetVertexArrayIndexed64iv = (PFNGLGETVERTEXARRAYINDEXED64IVPROC)load("glGetVertexArrayIndexed64iv");
	gl_loader_glCreateSamplers = (PFNGLCREATESAMPLERSPROC)load("glCreateSamplers");
	gl_loader_glCreateProgramPipelines = (PFNGLCREATEPROGRAMPIPELINESPROC)load("glCreateProgramPipelines");
	gl_loader_glCreateQueries = (PFNGLCREATEQUERIESPROC)load("glCreateQueries");
	gl_loader_glGetQueryBufferObjecti64v = (PFNGLGETQUERYBUFFEROBJECTI64VPROC)load("glGetQueryBufferObjecti64v");
	gl_loader_glGetQueryBufferObjectiv = (PFNGLGETQUERYBUFFEROBJECTIVPROC)load("glGetQueryBufferObjectiv");
	gl_loader_glGetQueryBufferObjectui64v = (PFNGLGETQUERYBUFFEROBJECTUI64VPROC)load("glGetQueryBufferObjectui64v");
	gl_loader_glGetQueryBufferObjectuiv = (PFNGLGETQUERYBUFFEROBJECTUIVPROC)load("glGetQueryBufferObjectuiv");
	gl_loader_glMemoryBarrierByRegion = (PFNGLMEMORYBARRIERBYREGIONPROC)load("glMemoryBarrierByRegion");
	gl_loader_glGetTextureSubImage = (PFNGLGETTEXTURESUBIMAGEPROC)load("glGetTextureSubImage");
	gl_loader_glGetCompressedTextureSubImage = (PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC)load("glGetCompressedTextureSubImage");
	gl_loader_glGetGraphicsResetStatus = (PFNGLGETGRAPHICSRESETSTATUSPROC)load("glGetGraphicsResetStatus");
	gl_loader_glGetnCompressedTexImage = (PFNGLGETNCOMPRESSEDTEXIMAGEPROC)load("glGetnCompressedTexImage");
	gl_loader_glGetnTexImage = (PFNGLGETNTEXIMAGEPROC)load("glGetnTexImage");
	gl_loader_glGetnUniformdv = (PFNGLGETNUNIFORMDVPROC)load("glGetnUniformdv");
	gl_loader_glGetnUniformfv = (PFNGLGETNUNIFORMFVPROC)load("glGetnUniformfv");
	gl_loader_glGetnUniformiv = (PFNGLGETNUNIFORMIVPROC)load("glGetnUniformiv");
	gl_loader_glGetnUniformuiv = (PFNGLGETNUNIFORMUIVPROC)load("glGetnUniformuiv");
	gl_loader_glReadnPixels = (PFNGLREADNPIXELSPROC)load("glReadnPixels");
	gl_loader_glGetnMapdv = (PFNGLGETNMAPDVPROC)load("glGetnMapdv");
	gl_loader_glGetnMapfv = (PFNGLGETNMAPFVPROC)load("glGetnMapfv");
	gl_loader_glGetnMapiv = (PFNGLGETNMAPIVPROC)load("glGetnMapiv");
	gl_loader_glGetnPixelMapfv = (PFNGLGETNPIXELMAPFVPROC)load("glGetnPixelMapfv");
	gl_loader_glGetnPixelMapuiv = (PFNGLGETNPIXELMAPUIVPROC)load("glGetnPixelMapuiv");
	gl_loader_glGetnPixelMapusv = (PFNGLGETNPIXELMAPUSVPROC)load("glGetnPixelMapusv");
	gl_loader_glGetnPolygonStipple = (PFNGLGETNPOLYGONSTIPPLEPROC)load("glGetnPolygonStipple");
	gl_loader_glGetnColorTable = (PFNGLGETNCOLORTABLEPROC)load("glGetnColorTable");
	gl_loader_glGetnConvolutionFilter = (PFNGLGETNCONVOLUTIONFILTERPROC)load("glGetnConvolutionFilter");
	gl_loader_glGetnSeparableFilter = (PFNGLGETNSEPARABLEFILTERPROC)load("glGetnSeparableFilter");
	gl_loader_glGetnHistogram = (PFNGLGETNHISTOGRAMPROC)load("glGetnHistogram");
	gl_loader_glGetnMinmax = (PFNGLGETNMINMAXPROC)load("glGetnMinmax");
	gl_loader_glTextureBarrier = (PFNGLTEXTUREBARRIERPROC)load("glTextureBarrier");
}
static int find_extensionsGL(void) {
	if (!get_exts()) return 0;
	(void)&has_ext;
	free_exts();
	return 1;
}

static void find_coreGL(void) {

    // ref: @elmindreda
    // https://github.com/elmindreda/greg/blob/master/templates/greg.c.in#L176
    // https://github.com/glfw/glfw/blob/master/src/context.c#L36
    
	int i, major, minor;

    const char* version;
    const char* prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL
    };

    version = (const char*) glGetString(GL_VERSION);
    if (!version) return;

    for (i = 0;  prefixes[i];  i++) {
        const size_t length = strlen(prefixes[i]);
        if (strncmp(version, prefixes[i], length) == 0) {
            version += length;
            break;
        }
    }

/* PR #18 */
#ifdef _MSC_VER
    sscanf_s(version, "%d.%d", &major, &minor);
#else
    sscanf(version, "%d.%d", &major, &minor);
#endif

    GLVersion.major = major; GLVersion.minor = minor;
    max_loaded_major = major; max_loaded_minor = minor;
	GLLOADER_GL_VERSION_1_0 = (major == 1 && minor >= 0) || major > 1;
	GLLOADER_GL_VERSION_1_1 = (major == 1 && minor >= 1) || major > 1;
	GLLOADER_GL_VERSION_1_2 = (major == 1 && minor >= 2) || major > 1;
	GLLOADER_GL_VERSION_1_3 = (major == 1 && minor >= 3) || major > 1;
	GLLOADER_GL_VERSION_1_4 = (major == 1 && minor >= 4) || major > 1;
	GLLOADER_GL_VERSION_1_5 = (major == 1 && minor >= 5) || major > 1;
	GLLOADER_GL_VERSION_2_0 = (major == 2 && minor >= 0) || major > 2;
	GLLOADER_GL_VERSION_2_1 = (major == 2 && minor >= 1) || major > 2;
	GLLOADER_GL_VERSION_3_0 = (major == 3 && minor >= 0) || major > 3;
	GLLOADER_GL_VERSION_3_1 = (major == 3 && minor >= 1) || major > 3;
	GLLOADER_GL_VERSION_3_2 = (major == 3 && minor >= 2) || major > 3;
	GLLOADER_GL_VERSION_3_3 = (major == 3 && minor >= 3) || major > 3;
	GLLOADER_GL_VERSION_4_0 = (major == 4 && minor >= 0) || major > 4;
	GLLOADER_GL_VERSION_4_1 = (major == 4 && minor >= 1) || major > 4;
	GLLOADER_GL_VERSION_4_2 = (major == 4 && minor >= 2) || major > 4;
	GLLOADER_GL_VERSION_4_3 = (major == 4 && minor >= 3) || major > 4;
	GLLOADER_GL_VERSION_4_4 = (major == 4 && minor >= 4) || major > 4;
	GLLOADER_GL_VERSION_4_5 = (major == 4 && minor >= 5) || major > 4;
	if (GLVersion.major > 4 || (GLVersion.major >= 4 && GLVersion.minor >= 5)) {
		max_loaded_major = 4;
		max_loaded_minor = 5;
	}
}

int gl_loaderLoadGLLoader(GLLOADERloadproc load) {
	GLVersion.major = 0; GLVersion.minor = 0;
	glGetString = (PFNGLGETSTRINGPROC)load("glGetString");
	if(glGetString == NULL) return 0;
	if(glGetString(GL_VERSION) == NULL) return 0;
	find_coreGL();
	load_GL_VERSION_1_0(load);
	load_GL_VERSION_1_1(load);
	load_GL_VERSION_1_2(load);
	load_GL_VERSION_1_3(load);
	load_GL_VERSION_1_4(load);
	load_GL_VERSION_1_5(load);
	load_GL_VERSION_2_0(load);
	load_GL_VERSION_2_1(load);
	load_GL_VERSION_3_0(load);
	load_GL_VERSION_3_1(load);
	load_GL_VERSION_3_2(load);
	load_GL_VERSION_3_3(load);
	load_GL_VERSION_4_0(load);
	load_GL_VERSION_4_1(load);
	load_GL_VERSION_4_2(load);
	load_GL_VERSION_4_3(load);
	load_GL_VERSION_4_4(load);
	load_GL_VERSION_4_5(load);

	if (!find_extensionsGL()) return 0;
	return GLVersion.major != 0 || GLVersion.minor != 0;
}

