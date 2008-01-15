/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2007 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#define _CRT_SECURE_NO_DEPRECATE // get rid of sprintf security warnings on VS2005

#include "config.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "alMain.h"
#include "alSource.h"
#include "AL/al.h"
#include "AL/alc.h"
#include "alThunk.h"
#include "alSource.h"
#include "alExtension.h"
#include "bs2b.h"

///////////////////////////////////////////////////////
// DEBUG INFORMATION

char _alDebug[256];

///////////////////////////////////////////////////////


#define EmptyFuncs { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
struct {
    const char *name;
    void (*Init)(BackendFuncs*);
    BackendFuncs Funcs;
} BackendList[] = {
#ifdef HAVE_ALSA
    { "alsa", alc_alsa_init, EmptyFuncs },
#endif
#ifdef HAVE_OSS
    { "oss", alc_oss_init, EmptyFuncs },
#endif
#ifdef HAVE_DSOUND
    { "dsound", alcDSoundInit, EmptyFuncs },
#endif
#ifdef HAVE_WINMM
    { "winmm", alcWinMMInit, EmptyFuncs },
#endif

    { "wave", alc_wave_init, EmptyFuncs },

    { NULL, NULL, EmptyFuncs }
};
#undef EmptyFuncs

///////////////////////////////////////////////////////

#define ALC_EFX_MAJOR_VERSION                              0x20001
#define ALC_EFX_MINOR_VERSION                              0x20002
#define ALC_MAX_AUXILIARY_SENDS                            0x20003

///////////////////////////////////////////////////////
// STRING and EXTENSIONS

typedef struct ALCextension_struct
{
    ALCchar        *extName;
    ALvoid        *address;
} ALCextension;

typedef struct ALCfunction_struct
{
    ALCchar        *funcName;
    ALvoid        *address;
} ALCfunction;

static ALCextension alcExtensions[] = {
    { "ALC_ENUMERATE_ALL_EXT", (ALvoid *) NULL },
    { "ALC_ENUMERATION_EXT",   (ALvoid *) NULL },
    { "ALC_EXT_CAPTURE",       (ALvoid *) NULL },
    { "ALC_EXT_EFX",           (ALvoid *) NULL },
    { NULL,                    (ALvoid *) NULL }
};

static ALCfunction  alcFunctions[] = {
    { "alcCreateContext",           (ALvoid *) alcCreateContext         },
    { "alcMakeContextCurrent",      (ALvoid *) alcMakeContextCurrent    },
    { "alcProcessContext",          (ALvoid *) alcProcessContext        },
    { "alcSuspendContext",          (ALvoid *) alcSuspendContext        },
    { "alcDestroyContext",          (ALvoid *) alcDestroyContext        },
    { "alcGetCurrentContext",       (ALvoid *) alcGetCurrentContext     },
    { "alcGetContextsDevice",       (ALvoid *) alcGetContextsDevice     },
    { "alcOpenDevice",              (ALvoid *) alcOpenDevice            },
    { "alcCloseDevice",             (ALvoid *) alcCloseDevice           },
    { "alcGetError",                (ALvoid *) alcGetError              },
    { "alcIsExtensionPresent",      (ALvoid *) alcIsExtensionPresent    },
    { "alcGetProcAddress",          (ALvoid *) alcGetProcAddress        },
    { "alcGetEnumValue",            (ALvoid *) alcGetEnumValue          },
    { "alcGetString",               (ALvoid *) alcGetString             },
    { "alcGetIntegerv",             (ALvoid *) alcGetIntegerv           },
    { "alcCaptureOpenDevice",       (ALvoid *) alcCaptureOpenDevice     },
    { "alcCaptureCloseDevice",      (ALvoid *) alcCaptureCloseDevice    },
    { "alcCaptureStart",            (ALvoid *) alcCaptureStart          },
    { "alcCaptureStop",             (ALvoid *) alcCaptureStop           },
    { "alcCaptureSamples",          (ALvoid *) alcCaptureSamples        },
    { NULL,                         (ALvoid *) NULL                     }
};

static ALenums enumeration[]={
    // Types
    { (ALchar *)"ALC_INVALID",                          ALC_INVALID                         },
    { (ALchar *)"ALC_FALSE",                            ALC_FALSE                           },
    { (ALchar *)"ALC_TRUE",                             ALC_TRUE                            },

    // ALC Properties
    { (ALchar *)"ALC_MAJOR_VERSION",                    ALC_MAJOR_VERSION                   },
    { (ALchar *)"ALC_MINOR_VERSION",                    ALC_MINOR_VERSION                   },
    { (ALchar *)"ALC_ATTRIBUTES_SIZE",                  ALC_ATTRIBUTES_SIZE                 },
    { (ALchar *)"ALC_ALL_ATTRIBUTES",                   ALC_ALL_ATTRIBUTES                  },
    { (ALchar *)"ALC_DEFAULT_DEVICE_SPECIFIER",         ALC_DEFAULT_DEVICE_SPECIFIER        },
    { (ALchar *)"ALC_DEVICE_SPECIFIER",                 ALC_DEVICE_SPECIFIER                },
    { (ALchar *)"ALC_ALL_DEVICES_SPECIFIER",            ALC_ALL_DEVICES_SPECIFIER           },
    { (ALchar *)"ALC_DEFAULT_ALL_DEVICES_SPECIFIER",    ALC_DEFAULT_ALL_DEVICES_SPECIFIER   },
    { (ALchar *)"ALC_EXTENSIONS",                       ALC_EXTENSIONS                      },
    { (ALchar *)"ALC_FREQUENCY",                        ALC_FREQUENCY                       },
    { (ALchar *)"ALC_REFRESH",                          ALC_REFRESH                         },
    { (ALchar *)"ALC_SYNC",                             ALC_SYNC                            },
    { (ALchar *)"ALC_MONO_SOURCES",                     ALC_MONO_SOURCES                    },
    { (ALchar *)"ALC_STEREO_SOURCES",                   ALC_STEREO_SOURCES                  },
    { (ALchar *)"ALC_CAPTURE_DEVICE_SPECIFIER",         ALC_CAPTURE_DEVICE_SPECIFIER        },
    { (ALchar *)"ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER", ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER},
    { (ALchar *)"ALC_CAPTURE_SAMPLES",                  ALC_CAPTURE_SAMPLES                 },

    // EFX Properties
    { (ALchar *)"ALC_EFX_MAJOR_VERSION",                ALC_EFX_MAJOR_VERSION               },
    { (ALchar *)"ALC_EFX_MINOR_VERSION",                ALC_EFX_MINOR_VERSION               },
    { (ALchar *)"ALC_MAX_AUXILIARY_SENDS",              ALC_MAX_AUXILIARY_SENDS             },

    // ALC Error Message
    { (ALchar *)"ALC_NO_ERROR",                         ALC_NO_ERROR                        },
    { (ALchar *)"ALC_INVALID_DEVICE",                   ALC_INVALID_DEVICE                  },
    { (ALchar *)"ALC_INVALID_CONTEXT",                  ALC_INVALID_CONTEXT                 },
    { (ALchar *)"ALC_INVALID_ENUM",                     ALC_INVALID_ENUM                    },
    { (ALchar *)"ALC_INVALID_VALUE",                    ALC_INVALID_VALUE                   },
    { (ALchar *)"ALC_OUT_OF_MEMORY",                    ALC_OUT_OF_MEMORY                   },
    { (ALchar *)NULL,                                   (ALenum)0 }
};
// Error strings
static const ALCchar alcNoError[] = "No Error";
static const ALCchar alcErrInvalidDevice[] = "Invalid Device";
static const ALCchar alcErrInvalidContext[] = "Invalid Context";
static const ALCchar alcErrInvalidEnum[] = "Invalid Enum";
static const ALCchar alcErrInvalidValue[] = "Invalid Value";
static const ALCchar alcErrOutOfMemory[] = "Out of Memory";

// Context strings
static ALCchar alcDeviceList[2048];
static ALCchar alcAllDeviceList[2048];
static ALCchar alcCaptureDeviceList[2048];
// Default is always the first in the list
static ALCchar *alcDefaultDeviceSpecifier = alcDeviceList;
static ALCchar *alcDefaultAllDeviceSpecifier = alcAllDeviceList;
static ALCchar *alcCaptureDefaultDeviceSpecifier = alcCaptureDeviceList;


static ALCchar alcExtensionList[] = "ALC_ENUMERATE_ALL_EXT ALC_ENUMERATION_EXT ALC_EXT_CAPTURE ALC_EXT_EFX";
static ALCint alcMajorVersion = 1;
static ALCint alcMinorVersion = 1;

static ALCint alcEFXMajorVersion = 1;
static ALCint alcEFXMinorVersion = 0;

///////////////////////////////////////////////////////


///////////////////////////////////////////////////////
// Global Variables

static ALCdevice *g_pDeviceList = NULL;
static ALCuint    g_ulDeviceCount = 0;

// Context List
static ALCcontext *g_pContextList = NULL;
static ALCuint     g_ulContextCount = 0;

// Context Error
static ALCenum g_eLastContextError = ALC_NO_ERROR;

///////////////////////////////////////////////////////


///////////////////////////////////////////////////////
// ALC Related helper functions

static void InitAL(void)
{
    static int done = 0;
    if(!done)
    {
        int i;
        const char *devs;

        InitializeCriticalSection(&_alMutex);
        ALTHUNK_INIT();
        ReadALConfig();

        devs = GetConfigValue(NULL, "drivers", "");
        if(devs[0])
        {
            int n;
            size_t len;
            const char *next = devs;

            i = 0;

            do {
                devs = next;
                next = strchr(devs, ',');

                if(!devs[0] || devs[0] == ',')
                    continue;

                len = (next ? ((size_t)(next-devs)) : strlen(devs));
                for(n = i;BackendList[n].Init;n++)
                {
                    if(len == strlen(BackendList[n].name) &&
                       strncmp(BackendList[n].name, devs, len) == 0)
                    {
                        const char *name = BackendList[i].name;
                        void (*Init)(BackendFuncs*) = BackendList[i].Init;

                        BackendList[i].name = BackendList[n].name;
                        BackendList[i].Init = BackendList[n].Init;

                        BackendList[n].name = name;
                        BackendList[n].Init = Init;

                        i++;
                    }
                }
            } while(next++);

            BackendList[i].name = NULL;
            BackendList[i].Init = NULL;
        }

        for(i = 0;BackendList[i].Init;i++)
            BackendList[i].Init(&BackendList[i].Funcs);
        done = 1;
    }
}

ALCchar *AppendDeviceList(char *name)
{
    static int pos;
    ALCchar *ret = alcDeviceList+pos;
    pos += snprintf(alcDeviceList+pos, sizeof(alcDeviceList)-pos, "%s", name) + 1;
    return ret;
}

ALCchar *AppendAllDeviceList(char *name)
{
    static int pos;
    ALCchar *ret = alcAllDeviceList+pos;
    pos += snprintf(alcAllDeviceList+pos, sizeof(alcAllDeviceList)-pos, "%s", name) + 1;
    return ret;
}

ALCchar *AppendCaptureDeviceList(char *name)
{
    static int pos;
    ALCchar *ret = alcCaptureDeviceList+pos;
    pos += snprintf(alcCaptureDeviceList+pos, sizeof(alcCaptureDeviceList)-pos, "%s", name) + 1;
    return ret;
}

/*
    IsContext

    Check pContext is a valid Context pointer
*/
static ALCboolean IsContext(ALCcontext *pContext)
{
    ALCcontext *pTempContext;

    pTempContext = g_pContextList;
    while (pTempContext && pTempContext != pContext)
        pTempContext = pTempContext->next;

    return (pTempContext ? ALC_TRUE : ALC_FALSE);
}


/*
    SetALCError

    Store latest ALC Error
*/
ALCvoid SetALCError(ALenum errorCode)
{
    g_eLastContextError = errorCode;
}


/*
    SuspendContext

    Thread-safe entry
*/
ALCvoid SuspendContext(ALCcontext *pContext)
{
    (void)pContext;
    EnterCriticalSection(&_alMutex);
}


/*
    ProcessContext

    Thread-safe exit
*/
ALCvoid ProcessContext(ALCcontext *pContext)
{
    (void)pContext;
    LeaveCriticalSection(&_alMutex);
}


/*
    InitContext

    Initialize Context variables
*/
static ALvoid InitContext(ALCcontext *pContext)
{
    int level;

    //Initialise listener
    pContext->Listener.Gain = 1.0f;
    pContext->Listener.MetersPerUnit = 1.0f;
    pContext->Listener.Position[0] = 0.0f;
    pContext->Listener.Position[1] = 0.0f;
    pContext->Listener.Position[2] = 0.0f;
    pContext->Listener.Velocity[0] = 0.0f;
    pContext->Listener.Velocity[1] = 0.0f;
    pContext->Listener.Velocity[2] = 0.0f;
    pContext->Listener.Forward[0] = 0.0f;
    pContext->Listener.Forward[1] = 0.0f;
    pContext->Listener.Forward[2] = -1.0f;
    pContext->Listener.Up[0] = 0.0f;
    pContext->Listener.Up[1] = 1.0f;
    pContext->Listener.Up[2] = 0.0f;

    //Validate pContext
    pContext->LastError = AL_NO_ERROR;
    pContext->InUse = AL_FALSE;

    //Set output format
    pContext->Frequency = pContext->Device->Frequency;

    //Set globals
    pContext->DistanceModel = AL_INVERSE_DISTANCE_CLAMPED;
    pContext->DopplerFactor = 1.0f;
    pContext->DopplerVelocity = 1.0f;
    pContext->flSpeedOfSound = SPEEDOFSOUNDMETRESPERSEC;

    pContext->lNumStereoSources = 1;
    pContext->lNumMonoSources = pContext->Device->MaxNoOfSources - pContext->lNumStereoSources;

    strcpy(pContext->ExtensionList, "AL_EXT_EXPONENT_DISTANCE AL_EXT_FLOAT32 AL_EXT_IMA4 AL_EXT_LINEAR_DISTANCE AL_EXT_MCFORMATS AL_EXT_OFFSET AL_LOKI_quadriphonic");

    level = GetConfigValueInt(NULL, "cf_level", 0);
    if(level > 0 && level <= 6)
    {
        pContext->bs2b = calloc(1, sizeof(*pContext->bs2b));
        bs2b_set_srate(pContext->bs2b, pContext->Frequency);
        bs2b_set_level(pContext->bs2b, level);
    }
}


/*
    ExitContext

    Clean up Context, destroy any remaining Sources
*/
static ALCvoid ExitContext(ALCcontext *pContext)
{
    unsigned int i;
    ALsource *ALSource;
    ALsource *ALTempSource;

#ifdef _DEBUG
    if (pContext->SourceCount>0)
        AL_PRINT("alcDestroyContext() %d Source(s) NOT deleted\n", pContext->SourceCount);
#endif

    // Free all the Sources still remaining
    ALSource = pContext->Source;
    for (i = 0; i < pContext->SourceCount; i++)
    {
        ALTempSource = ALSource->next;
        ALTHUNK_REMOVEENTRY(ALSource->source);
        memset(ALSource, 0, sizeof(ALsource));
        free(ALSource);
        ALSource = ALTempSource;
    }

    //Invalidate context
    pContext->LastError = AL_NO_ERROR;
    pContext->InUse = AL_FALSE;

    free(pContext->bs2b);
    pContext->bs2b = NULL;
}

///////////////////////////////////////////////////////


///////////////////////////////////////////////////////
// ALC Functions calls


// This should probably move to another c file but for now ...
ALCAPI ALCdevice* ALCAPIENTRY alcCaptureOpenDevice(const ALCchar *deviceName, ALCuint frequency, ALCenum format, ALCsizei SampleSize)
{
    ALCboolean DeviceFound = ALC_FALSE;
    ALCdevice *pDevice = NULL;
    ALCint i;

    InitAL();

    if(deviceName && !deviceName[0])
        deviceName = NULL;

    pDevice = malloc(sizeof(ALCdevice));
    if (pDevice)
    {
        if (SampleSize > 0)
        {
            //Initialise device structure
            memset(pDevice, 0, sizeof(ALCdevice));

            //Validate device
            pDevice->InUse = AL_TRUE;
            pDevice->IsCaptureDevice = AL_TRUE;

            pDevice->Frequency = frequency;
            pDevice->Format = format;
            pDevice->Channels = aluChannelsFromFormat(format);
            pDevice->FrameSize = aluBytesFromFormat(format) *
                                 pDevice->Channels;

            for(i = 0;BackendList[i].Init;i++)
            {
                pDevice->Funcs = &BackendList[i].Funcs;
                if(ALCdevice_OpenCapture(pDevice, deviceName, frequency, format, SampleSize))
                {
                    SuspendContext(NULL);
                    pDevice->next = g_pDeviceList;
                    g_pDeviceList = pDevice;
                    g_ulDeviceCount++;
                    ProcessContext(NULL);

                    DeviceFound = ALC_TRUE;
                    break;
                }
            }
        }
        else
            SetALCError(ALC_INVALID_VALUE);

        if(!DeviceFound)
        {
            free(pDevice);
            pDevice = NULL;
        }
    }
    else
        SetALCError(ALC_OUT_OF_MEMORY);

    return pDevice;
}

ALCAPI ALCboolean ALCAPIENTRY alcCaptureCloseDevice(ALCdevice *pDevice)
{
    ALCboolean bReturn = ALC_FALSE;
    ALCdevice **list;

    if ((pDevice)&&(pDevice->IsCaptureDevice))
    {
        SuspendContext(NULL);

        list = &g_pDeviceList;
        while(*list != pDevice)
            list = &(*list)->next;

        *list = (*list)->next;
        g_ulDeviceCount--;

        ProcessContext(NULL);

        ALCdevice_CloseCapture(pDevice);
        free(pDevice);

        bReturn = ALC_TRUE;
    }
    else
        SetALCError(ALC_INVALID_DEVICE);

    return bReturn;
}

ALCAPI void ALCAPIENTRY alcCaptureStart(ALCdevice *pDevice)
{
    if ((pDevice)&&(pDevice->IsCaptureDevice))
        ALCdevice_StartCapture(pDevice);
    else
        SetALCError(ALC_INVALID_DEVICE);
}

ALCAPI void ALCAPIENTRY alcCaptureStop(ALCdevice *pDevice)
{
    if ((pDevice)&&(pDevice->IsCaptureDevice))
        ALCdevice_StopCapture(pDevice);
    else
        SetALCError(ALC_INVALID_DEVICE);
}

ALCAPI void ALCAPIENTRY alcCaptureSamples(ALCdevice *pDevice, ALCvoid *pBuffer, ALCsizei lSamples)
{
    if ((pDevice) && (pDevice->IsCaptureDevice))
        ALCdevice_CaptureSamples(pDevice, pBuffer, lSamples);
    else
        SetALCError(ALC_INVALID_DEVICE);
}

/*
    alcGetError

    Return last ALC generated error code
*/
ALCAPI ALCenum ALCAPIENTRY alcGetError(ALCdevice *device)
{
    ALCenum errorCode;

    (void)device;

    errorCode = g_eLastContextError;
    g_eLastContextError = ALC_NO_ERROR;
    return errorCode;
}


/*
    alcSuspendContext

    Not functional
*/
ALCAPI ALCvoid ALCAPIENTRY alcSuspendContext(ALCcontext *pContext)
{
    // Not a lot happens here !
    (void)pContext;
}


/*
    alcProcessContext

    Not functional
*/
ALCAPI ALCvoid ALCAPIENTRY alcProcessContext(ALCcontext *pContext)
{
    // Not a lot happens here !
    (void)pContext;
}


/*
    alcGetString

    Returns information about the Device, and error strings
*/
ALCAPI const ALCchar* ALCAPIENTRY alcGetString(ALCdevice *pDevice,ALCenum param)
{
    const ALCchar *value = NULL;

    InitAL();

    switch (param)
    {
    case ALC_NO_ERROR:
        value = alcNoError;
        break;

    case ALC_INVALID_ENUM:
        value = alcErrInvalidEnum;
        break;

    case ALC_INVALID_VALUE:
        value = alcErrInvalidValue;
        break;

    case ALC_INVALID_DEVICE:
        value = alcErrInvalidDevice;
        break;

    case ALC_INVALID_CONTEXT:
        value = alcErrInvalidContext;
        break;

    case ALC_OUT_OF_MEMORY:
        value = alcErrOutOfMemory;
        break;

    case ALC_DEFAULT_DEVICE_SPECIFIER:
        value = alcDefaultDeviceSpecifier;
        break;

    case ALC_DEVICE_SPECIFIER:
        if (pDevice)
            value = pDevice->szDeviceName;
        else
            value = alcDeviceList;
        break;

    case ALC_ALL_DEVICES_SPECIFIER:
        value = alcAllDeviceList;
        break;

    case ALC_DEFAULT_ALL_DEVICES_SPECIFIER:
        value = alcDefaultAllDeviceSpecifier;
        break;

    case ALC_CAPTURE_DEVICE_SPECIFIER:
        if (pDevice)
            value = pDevice->szDeviceName;
        else
            value = alcCaptureDeviceList;
        break;

    case ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER:
        value = alcCaptureDefaultDeviceSpecifier;
        break;

    case ALC_EXTENSIONS:
        value = alcExtensionList;
        break;

    default:
        SetALCError(ALC_INVALID_ENUM);
        break;
    }

    return value;
}


/*
    alcGetIntegerv

    Returns information about the Device and the version of Open AL
*/
ALCAPI ALCvoid ALCAPIENTRY alcGetIntegerv(ALCdevice *device,ALCenum param,ALsizei size,ALCint *data)
{
    InitAL();

    if ((device)&&(device->IsCaptureDevice))
    {
        SuspendContext(NULL);

        // Capture device
        switch (param)
        {
        case ALC_CAPTURE_SAMPLES:
            if ((size) && (data))
                *data = ALCdevice_AvailableSamples(device);
            else
                SetALCError(ALC_INVALID_VALUE);
            break;

        default:
            SetALCError(ALC_INVALID_ENUM);
            break;
        }

        ProcessContext(NULL);
    }
    else
    {
        if(data)
        {
            // Playback Device
            switch (param)
            {
                case ALC_MAJOR_VERSION:
                    if(!size)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                        *data = alcMajorVersion;
                    break;

                case ALC_MINOR_VERSION:
                    if(!size)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                        *data = alcMinorVersion;
                    break;

                case ALC_EFX_MAJOR_VERSION:
                    if(!size)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                        *data = alcEFXMajorVersion;
                    break;

                case ALC_EFX_MINOR_VERSION:
                    if(!size)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                        *data = alcEFXMinorVersion;
                    break;

                case ALC_MAX_AUXILIARY_SENDS:
                    if(!size)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                        *data = MAX_SENDS;
                    break;

                case ALC_ATTRIBUTES_SIZE:
                    if(!device)
                        SetALCError(ALC_INVALID_DEVICE);
                    else if(!size)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                        *data = 12;
                    break;

                case ALC_ALL_ATTRIBUTES:
                    if(!device)
                        SetALCError(ALC_INVALID_DEVICE);
                    else if (size < 7)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                    {
                        int i = 0;

                        data[i++] = ALC_FREQUENCY;
                        data[i++] = device->Frequency;

                        data[i++] = ALC_REFRESH;
                        data[i++] = device->Frequency / device->UpdateFreq;

                        data[i++] = ALC_SYNC;
                        data[i++] = ALC_FALSE;

                        SuspendContext(NULL);
                        if(device->Context && size >= 12)
                        {
                            data[i++] = ALC_MONO_SOURCES;
                            data[i++] = device->Context->lNumMonoSources;

                            data[i++] = ALC_STEREO_SOURCES;
                            data[i++] = device->Context->lNumStereoSources;

                            data[i++] = ALC_MAX_AUXILIARY_SENDS;
                            data[i++] = MAX_SENDS;
                        }
                        ProcessContext(NULL);

                        data[i++] = 0;
                    }
                    break;

                case ALC_FREQUENCY:
                    if(!device)
                        SetALCError(ALC_INVALID_DEVICE);
                    else if(!size)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                        *data = device->Frequency;
                    break;

                case ALC_REFRESH:
                    if(!device)
                        SetALCError(ALC_INVALID_DEVICE);
                    else if(!size)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                        *data = device->Frequency / device->UpdateFreq;
                    break;

                case ALC_SYNC:
                    if(!device)
                        SetALCError(ALC_INVALID_DEVICE);
                    else if(!size)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                        *data = ALC_FALSE;
                    break;

                case ALC_MONO_SOURCES:
                    if(!device || !device->Context)
                        SetALCError(ALC_INVALID_DEVICE);
                    else if (size != 1)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                        *data = device->Context->lNumMonoSources;
                    break;

                case ALC_STEREO_SOURCES:
                    if(!device || !device->Context)
                        SetALCError(ALC_INVALID_DEVICE);
                    else if (size != 1)
                        SetALCError(ALC_INVALID_VALUE);
                    else
                        *data = device->Context->lNumStereoSources;
                    break;

                default:
                    SetALCError(ALC_INVALID_ENUM);
                    break;
            }
        }
        else if(size)
            SetALCError(ALC_INVALID_VALUE);
    }

    return;
}


/*
    alcIsExtensionPresent

    Determines if there is support for a particular extension
*/
ALCAPI ALCboolean ALCAPIENTRY alcIsExtensionPresent(ALCdevice *device, const ALCchar *extName)
{
    ALCboolean bResult = ALC_FALSE;
    ALsizei i = 0;

    (void)device;

    if (extName)
    {
        while(alcExtensions[i].extName &&
              strcasecmp(alcExtensions[i].extName,extName) != 0)
            i++;

        if (alcExtensions[i].extName)
            bResult = ALC_TRUE;
    }
    else
        SetALCError(ALC_INVALID_VALUE);

    return bResult;
}


/*
    alcGetProcAddress

    Retrieves the function address for a particular extension function
*/
ALCAPI ALCvoid *  ALCAPIENTRY alcGetProcAddress(ALCdevice *device, const ALCchar *funcName)
{
    ALCvoid *pFunction = NULL;
    ALsizei i = 0;

    (void)device;

    if (funcName)
    {
        while(alcFunctions[i].funcName &&
              strcmp(alcFunctions[i].funcName,funcName) != 0)
            i++;
        pFunction = alcFunctions[i].address;
    }
    else
        SetALCError(ALC_INVALID_VALUE);

    return pFunction;
}


/*
    alcGetEnumValue

    Get the value for a particular ALC Enumerated Value
*/
ALCAPI ALCenum ALCAPIENTRY alcGetEnumValue(ALCdevice *device, const ALCchar *enumName)
{
    ALsizei i = 0;
    ALCenum val;

    (void)device;

    while ((enumeration[i].enumName)&&(strcmp(enumeration[i].enumName,enumName)))
        i++;
    val = enumeration[i].value;

    if(!enumeration[i].enumName)
        SetALCError(ALC_INVALID_VALUE);

    return val;
}


/*
    alcCreateContext

    Create and attach a Context to a particular Device.
*/
ALCAPI ALCcontext* ALCAPIENTRY alcCreateContext(ALCdevice *device, const ALCint *attrList)
{
    ALCcontext *ALContext = NULL;
    ALuint      ulAttributeIndex, ulRequestedStereoSources;

    if ((device)&&(!device->IsCaptureDevice))
    {
        // Reset Context Last Error code
        g_eLastContextError = ALC_NO_ERROR;

        // Current implementation only allows one Context per Device
        if(!device->Context)
        {
            ALContext = calloc(1, sizeof(ALCcontext));
            if(!ALContext)
            {
                SetALCError(ALC_OUT_OF_MEMORY);
                return NULL;
            }

            ALContext->Device = device;
            InitContext(ALContext);

            device->Context = ALContext;

            SuspendContext(NULL);

            ALContext->next = g_pContextList;
            g_pContextList = ALContext;
            g_ulContextCount++;

            ProcessContext(NULL);

            // Check for Voice Count attributes
            if (attrList)
            {
                ulAttributeIndex = 0;
                while ((ulAttributeIndex < 10) && (attrList[ulAttributeIndex]))
                {
                    if (attrList[ulAttributeIndex] == ALC_STEREO_SOURCES)
                    {
                        ulRequestedStereoSources = attrList[ulAttributeIndex + 1];

                        if (ulRequestedStereoSources > ALContext->Device->MaxNoOfSources)
                            ulRequestedStereoSources = ALContext->Device->MaxNoOfSources;

                        ALContext->lNumStereoSources = ulRequestedStereoSources;
                        ALContext->lNumMonoSources = ALContext->Device->MaxNoOfSources - ALContext->lNumStereoSources;
                        break;
                    }

                    ulAttributeIndex += 2;
                }
            }
        }
        else
        {
            SetALCError(ALC_INVALID_VALUE);
            ALContext = NULL;
        }
    }
    else
        SetALCError(ALC_INVALID_DEVICE);

    return ALContext;
}


/*
    alcDestroyContext

    Remove a Context
*/
ALCAPI ALCvoid ALCAPIENTRY alcDestroyContext(ALCcontext *context)
{
    ALCcontext **list;

    // Lock context list
    SuspendContext(NULL);

    if (IsContext(context))
    {
        // Lock context
        SuspendContext(context);

        context->Device->Context = NULL;

        list = &g_pContextList;
        while(*list != context)
            list = &(*list)->next;

        *list = (*list)->next;
        g_ulContextCount--;

        // Unlock context
        ProcessContext(context);

        ExitContext(context);

        // Free memory (MUST do this after ProcessContext)
        memset(context, 0, sizeof(ALCcontext));
        free(context);
    }
    else
        SetALCError(ALC_INVALID_CONTEXT);

    ProcessContext(NULL);
}


/*
    alcGetCurrentContext

    Returns the currently active Context
*/
ALCAPI ALCcontext * ALCAPIENTRY alcGetCurrentContext(ALCvoid)
{
    ALCcontext *pContext = NULL;

    SuspendContext(NULL);

    pContext = g_pContextList;
    while ((pContext) && (!pContext->InUse))
        pContext = pContext->next;

    ProcessContext(NULL);

    return pContext;
}


/*
    alcGetContextsDevice

    Returns the Device that a particular Context is attached to
*/
ALCAPI ALCdevice* ALCAPIENTRY alcGetContextsDevice(ALCcontext *pContext)
{
    ALCdevice *pDevice = NULL;

    SuspendContext(NULL);
    if (IsContext(pContext))
        pDevice = pContext->Device;
    else
        SetALCError(ALC_INVALID_CONTEXT);
    ProcessContext(NULL);

    return pDevice;
}


/*
    alcMakeContextCurrent

    Makes the given Context the active Context
*/
ALCAPI ALCboolean ALCAPIENTRY alcMakeContextCurrent(ALCcontext *context)
{
    ALCcontext *ALContext;
    ALboolean bReturn = AL_TRUE;

    SuspendContext(NULL);

    // context must be a valid Context or NULL
    if ((IsContext(context)) || (context == NULL))
    {
        if ((ALContext=alcGetCurrentContext()))
        {
            SuspendContext(ALContext);
            ALContext->InUse=AL_FALSE;
            ProcessContext(ALContext);
        }

        if ((ALContext=context) && (ALContext->Device))
        {
            SuspendContext(ALContext);
            ALContext->InUse=AL_TRUE;
            ProcessContext(ALContext);
        }
    }
    else
    {
        SetALCError(ALC_INVALID_CONTEXT);
        bReturn = AL_FALSE;
    }

    ProcessContext(NULL);

    return bReturn;
}


/*
    alcOpenDevice

    Open the Device specified.
*/
ALCAPI ALCdevice* ALCAPIENTRY alcOpenDevice(const ALCchar *deviceName)
{
    ALboolean bDeviceFound = AL_FALSE;
    ALCdevice *device;
    ALint i;

    InitAL();

    if(deviceName && !deviceName[0])
        deviceName = NULL;

    device = malloc(sizeof(ALCdevice));
    if (device)
    {
        const char *fmt;

        //Initialise device structure
        memset(device, 0, sizeof(ALCdevice));

        //Validate device
        device->InUse = AL_TRUE;
        device->IsCaptureDevice = AL_FALSE;

        //Set output format
        device->Frequency = GetConfigValueInt(NULL, "frequency", SWMIXER_OUTPUT_RATE);
        if((ALint)device->Frequency <= 0)
            device->Frequency = SWMIXER_OUTPUT_RATE;

        fmt = GetConfigValue(NULL, "format", "AL_FORMAT_STEREO16");
        if(fmt[0])
            device->Format = alGetEnumValue(fmt);

        device->Channels = aluChannelsFromFormat(device->Format);
        if(!device->Channels)
        {
            device->Format = AL_FORMAT_STEREO16;
            device->Channels = 2;
            device->FrameSize = 4;
        }
        else
            device->FrameSize = aluBytesFromFormat(device->Format) *
                                device->Channels;

        device->UpdateFreq = GetConfigValueInt(NULL, "refresh", 0);
        if((ALint)device->UpdateFreq <= 0)
            device->UpdateFreq = 8192 * device->Frequency / 22050;

        // Find a playback device to open
        for(i = 0;BackendList[i].Init;i++)
        {
            device->Funcs = &BackendList[i].Funcs;
            if(ALCdevice_OpenPlayback(device, deviceName))
            {
                SuspendContext(NULL);
                device->next = g_pDeviceList;
                g_pDeviceList = device;
                g_ulDeviceCount++;
                ProcessContext(NULL);

                bDeviceFound = AL_TRUE;
                break;
            }
        }

        if (!bDeviceFound)
        {
            // No suitable output device found
            free(device);
            device = NULL;
        }
    }

    return device;
}


/*
    alcCloseDevice

    Close the specified Device
*/
ALCAPI ALCboolean ALCAPIENTRY alcCloseDevice(ALCdevice *pDevice)
{
    ALCboolean bReturn = ALC_FALSE;
    ALCdevice **list;

    if ((pDevice)&&(!pDevice->IsCaptureDevice))
    {
        SuspendContext(NULL);

        list = &g_pDeviceList;
        while(*list != pDevice)
            list = &(*list)->next;

        *list = (*list)->next;
        g_ulDeviceCount--;

        ProcessContext(NULL);

        if(pDevice->Context)
            alcDestroyContext(pDevice->Context);
        ALCdevice_ClosePlayback(pDevice);

        //Release device structure
        memset(pDevice, 0, sizeof(ALCdevice));
        free(pDevice);

        bReturn = ALC_TRUE;
    }
    else
        SetALCError(ALC_INVALID_DEVICE);

    return bReturn;
}


ALCvoid ReleaseALC(ALCvoid)
{
    ALCdevice *Dev;

#ifdef _DEBUG
    if(g_ulContextCount > 0)
        AL_PRINT("exit() %u device(s) and %u context(s) NOT deleted\n", g_ulDeviceCount, g_ulContextCount);
#endif

    while(g_pDeviceList)
    {
        Dev = g_pDeviceList;
        g_pDeviceList = g_pDeviceList->next;
        if(Dev->IsCaptureDevice)
            alcCaptureCloseDevice(Dev);
        else
            alcCloseDevice(Dev);
    }
}

///////////////////////////////////////////////////////
