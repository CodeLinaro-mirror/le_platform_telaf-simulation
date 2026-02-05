/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @page c_taf_audio Audio Service
 *
 * @rst :ref:`API reference <File taf_audio_interface.h>` @endrst
 *
 * @rst :ref:`HAL APIs <File tafHalAudio.h>` @endrst
 *
 * <HR>
 *
 * The Audio service APIs help to do audio operations on the NAD/TCU.
 *
 * @section taf_audio_binding IPC interfaces binding
 *
 *
 * The following example illustrates how to bind to the Audio service.
 * @verbatim
   bindings:
   {
       clientExe.clientComponent.taf_audio -> tafAudioSvc.taf_audio
   }
   @endverbatim
 *
 * @section c_taf_audio_openRoute Open/Close Route for the requested route type and mode
 *
 * TelAF Audio Service helps the user to open the audio connection for the requested route
 * type like ecall, non-ecall or playback/capture or playback of a playlist of audio files and
 * mode type as voice call, playback, record. It supports DTMF detection and generation on the Rx
 * path of voice call.
 *
 * Once route is opened, connect the sink reference and source reference to the
 * respective Rx/Tx connectors in case of voice call.
 * If the audio VHAL module is available, the VHAL OpenRoute will get called after the requested
 * audio route for the mode is established.
 *
 * Reference created by OpenRoute is used to close the route using taf_audio_CloseRoute() API.
 * @subsection c_open_route_voice_call Open route for voice call audio connection
 *
 * @verbatim

    taf_audio_StreamRef_t sinkRef = NULL;
    taf_audio_StreamRef_t sourceRef = NULL;
    taf_audio_RouteRef_t routeRef = NULL;
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    if(routeRef != NULL) {
        LE_INFO("OpenRoute successfull sinkRef %p sourceRef %p", sinkRef, sourceRef);
    }

    taf_audio_ConnectorRef_t rxConn = taf_audio_CreateConnector();
    taf_audio_ConnectorRef_t txConn = taf_audio_CreateConnector();

    taf_audio_StreamRef_t rxStreamRef = taf_audio_OpenModemVoiceRx(1);
    // true to enable ECNR configuration.
    taf_audio_StreamRef_t txStreamRef = taf_audio_OpenModemVoiceTx(1, true);

    res = taf_audio_Connect(rxConn, sinkRef);
    if(res == LE_OK) {
        LE_INFO("sinkRef connected successfully to rxConn" );
    }

    res = taf_audio_Connect(txConn, sourceRef);
    if(res == LE_OK) {
        LE_INFO("sourceRef connected successfully to txConn" );
    }

    res = taf_audio_Connect(rxConn, rxStreamRef);
    if(res == LE_OK) {
        LE_INFO("rxStreamRef connected successfully to rxConn" );
    }

    res = taf_audio_Connect(txConn, txStreamRef);
    if(res == LE_OK) {
        LE_INFO("txStreamRef connected successfully to txConn" );
    }

    taf_audio_Disconnect(txConn, txStreamRef);

    taf_audio_Disconnect(rxConn, sinkRef);

    taf_audio_Disconnect(txConn, sourceRef);

    taf_audio_Disconnect(rxConn, rxStreamRef);

    res = taf_audio_CloseRoute(routeRef);
    if(res == LE_OK) {
        LE_INFO("taf_audio_CloseRoute successful" );
    }

   @endverbatim
 *
 * To play the file(s) in the local speaker(local playback) during active voice call.
 * @verbatim
    le_thread_Ref_t Player_thread_ref;
    le_sem_Ref_t tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);
    taf_audio_StreamRef_t playerRef = NULL;
    const char* srcPath = "/data/test.amr";
    void* Test_taf_audio_AddHandler(void* ctxPtr)
    {
        le_sem_Ref_t sem = NULL;
        taf_audio_ConnectService();

        taf_audio_StreamRef_t streamRef = (taf_audio_StreamRef_t)ctxPtr;
        MediaHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
        if(MediaHandlerRef != NULL)
            LE_INFO("Successfully registered for media handler");

        sem = le_sem_FindSemaphore("tafAudioAppSem");
        if(sem != NULL)
        {
            le_sem_Post(sem);
        }
        else
        {
            LE_ERROR("tafAudioAppSem is NULL!");
        }
        le_event_RunLoop();
        return NULL;
    }
    static void MyMediaEventHandler
    (
        taf_audio_StreamRef_t          streamRef,
        taf_audio_MediaEvent_t         event,
        void*                               contextPtr
    )
    {
        switch(event)
        {
            case TAF_AUDIO_MEDIA_ENDED:
                LE_INFO(" Playback completed");
                break;
            case TAF_AUDIO_MEDIA_STOPPED:
                LE_INFO(" Playback/capture stopped");
                le_sem_Post(tafAudioAppSem);
                break;
            case TAF_AUDIO_MEDIA_ERROR:
                LE_INFO("File event is TAF_AUDIO_MEDIA_ERROR.");
                break;
            case TAF_AUDIO_MEDIA_NO_MORE_SAMPLES:
                LE_INFO("File event is TAF_AUDIO_MEDIA_NO_MORE_SAMPLES.");
                break;
            default:
                LE_INFO("File event is %d", event);
                break;
        }
    }
    void TEST_LOCAL_PLAYBACK_VOICE_ACTIVE()
    {
        taf_audio_ConnectorRef_t playerConnRef = taf_audio_CreateConnector();

        playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
        //sinkRef should be the same stream reference sent in VOICE_CALL OpenRoute.
        res = taf_audio_Connect(playerConnRef, sinkRef);
        if(res != LE_OK)
        {
            LE_ERROR("Failed to connect sinkRef to playerConnRef");
        }

        res = taf_audio_Connect(playerConnRef, playerRef);
        if(res != LE_OK)
        {
            LE_ERROR("Failed to connect playerRef to playerConnRef");
        }
        Player_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
                Test_taf_audio_AddHandler, (void*)playerRef);
        le_thread_Start(Player_thread_ref);

        le_sem_Wait(tafAudioAppSem);
        res = taf_audio_PlayFile(playerRef, srcPath);
        if (res != LE_OK)
        {
            LE_ERROR("Failed to start the playback");
        }
        res = taf_audio_Stop(playerRef);
        if (res != LE_OK)
        {
            LE_ERROR("Failed to stop the playback");
        }
        // Wait till MEDIA_STOPPED callback is received.

        // PlayFileList API can also be used to play the list of files locally during active
        // voice call
        taf_audio_PlayFileConfig_t playFileConfig[1] = {0};
        snprintf(playFileConfig[0].srcPath, sizeof(playFileConfig[0].srcPath), "%s", srcPath);
        playFileConfig[0].repeat = repeat;

        res = taf_audio_PlayFileList(playerRef, playFileConfig,
                sizeof(playFileConfig)/sizeof(taf_audio_PlayFileConfig_t));
        if (res != LE_OK)
        {
            LE_ERROR("Failed to start the incall local file list playback");
        }

        res = taf_audio_Stop(playerRef);
        if (res != LE_OK)
        {
            LE_ERROR("Failed to stop the file playback");
        }
        // Wait till MEDIA_STOPPED callback is received.
        le_sem_Wait(tafAudioAppSem);

        taf_audio_Disconnect(playerConnRef, playerRef);

        taf_audio_Close(playerRef);

        taf_audio_RemoveMediaHandler(MediaHandlerRef);

        taf_audio_DeleteConnector(playerConnRef);
    }
   @endverbatim
 *
 * Recording the data from the local microphone during active voice call(incall uplink recording)
 * is not supported.
 *
 * Play the file(s) on the far end during active voice call(incall uplink playback).
 * Active voice call is mandatory for playback to be successful.
 * @verbatim
    le_thread_Ref_t Player_thread_ref;
    le_sem_Ref_t tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);
    taf_audio_StreamRef_t playerRef = NULL;
    const char* filePath = "/data/test.wav"
    void* Test_taf_audio_AddHandler(void* ctxPtr)
    {
        le_sem_Ref_t sem = NULL;
        taf_audio_ConnectService();

        taf_audio_StreamRef_t streamRef = (taf_audio_StreamRef_t)ctxPtr;
        MediaHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
        if(MediaHandlerRef != NULL)
            LE_INFO("Successfully registered for media handler");

        sem = le_sem_FindSemaphore("tafAudioAppSem");
        if(sem != NULL)
        {
            le_sem_Post(sem);
        }
        else
        {
            LE_ERROR("tafAudioAppSem is NULL!");
        }
        le_event_RunLoop();
        return NULL;
    }
    static void MyMediaEventHandler
    (
        taf_audio_StreamRef_t          streamRef,
        taf_audio_MediaEvent_t         event,
        void*                               contextPtr
    )
    {
        switch(event)
        {
            case TAF_AUDIO_MEDIA_ENDED:
                LE_INFO(" Playback completed");
                break;
            case TAF_AUDIO_MEDIA_STOPPED:
                LE_INFO(" Playback/capture stopped");
                le_sem_Post(tafAudioAppSem);
                break;
            case TAF_AUDIO_MEDIA_ERROR:
                LE_INFO("File event is TAF_AUDIO_MEDIA_ERROR.");
                break;
            case TAF_AUDIO_MEDIA_NO_MORE_SAMPLES:
                LE_INFO("File event is TAF_AUDIO_MEDIA_NO_MORE_SAMPLES.");
                break;
            default:
                LE_INFO("File event is %d", event);
                break;
        }
    }
    void TEST_INCALL_UPLINK_PLAYBACK()
    {
        taf_audio_ConnectorRef_t playerConnRef = taf_audio_CreateConnector();

        playerRef = taf_audio_OpenPlayer(TAF_AUDIO_TX);
        //txStreamRef should be the same stream reference created and connected for VOICE_CALL.
        res = taf_audio_Connect(playerConnRef, txStreamRef);
        if(res != LE_OK)
        {
            LE_ERROR("Failed to connect txStreamRef to playerConnRef");
        }

        res = taf_audio_Connect(playerConnRef, playerRef);
        if(res != LE_OK)
        {
            LE_ERROR("Failed to connect playerRef to playerConnRef");
        }
        Player_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
                Test_taf_audio_AddHandler, (void*)playerRef);
        le_thread_Start(Player_thread_ref);

        le_sem_Wait(tafAudioAppSem);
        res = taf_audio_PlayFile(playerRef, filePath);
        if (res != LE_OK)
        {
            LE_ERROR("Failed to start the playback");
        }
        res = taf_audio_Stop(playerRef);
        if (res != LE_OK)
        {
            LE_ERROR("Failed to stop the playback");
        }
        // Wait till MEDIA_STOPPED callback is received.

        // PlayFileList API can also be used to play the list of files locally during active
        // voice call
        const char* srcPath = "/data/test.amr";
        taf_audio_PlayFileConfig_t playFileConfig[1] = {0};
        snprintf(playFileConfig[0].srcPath, sizeof(playFileConfig[0].srcPath), "%s", srcPath);
        playFileConfig[0].repeat = repeat;

        res = taf_audio_PlayFileList(playerRef, playFileConfig,
                sizeof(playFileConfig)/sizeof(taf_audio_PlayFileConfig_t));
        if (res != LE_OK)
        {
            LE_ERROR("Failed to start the incall local file list playback");
        }

        res = taf_audio_Stop(playerRef);
        if (res != LE_OK)
        {
            LE_ERROR("Failed to stop the file playback");
        }
        // Wait till MEDIA_STOPPED callback is received.
        le_sem_Wait(tafAudioAppSem);

        taf_audio_Disconnect(playerConnRef, playerRef);

        taf_audio_Close(playerRef);

        taf_audio_RemoveMediaHandler(MediaHandlerRef);

        taf_audio_DeleteConnector(playerConnRef);
    }
   @endverbatim
 *
 * Record the packets received at the modem RX during active voice call. Active voice call is
 * mandatory for recording to be successful. File will be recorded in PCM format and with the
 * channel type both left and right.
 *
 * @verbatim
    le_thread_Ref_t Recorder_thread_ref;
    le_sem_Ref_t tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);
    const char* recordfilePath = "/data/record.wav"
    taf_audio_StreamRef_t recorderRef = NULL;
    void* Test_taf_audio_AddHandler(void* ctxPtr)
    {
        le_sem_Ref_t sem = NULL;
        taf_audio_ConnectService();

        taf_audio_StreamRef_t streamRef = (taf_audio_StreamRef_t)ctxPtr;
        MediaHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
        if(MediaHandlerRef != NULL)
            LE_INFO("Successfully registered for media handler");

        sem = le_sem_FindSemaphore("tafAudioAppSem");
        if(sem != NULL)
        {
            le_sem_Post(sem);
        }
        else
        {
            LE_ERROR("tafAudioAppSem is NULL!");
        }
        le_event_RunLoop();
        return NULL;
    }
    static void MyMediaEventHandler
    (
        taf_audio_StreamRef_t          streamRef,
        taf_audio_MediaEvent_t         event,
        void*                               contextPtr
    )
    {
        switch(event)
        {
            case TAF_AUDIO_MEDIA_ENDED:
                LE_INFO(" Playback completed");
                break;
            case TAF_AUDIO_MEDIA_STOPPED:
                LE_INFO(" Playback/capture stopped");
                le_sem_Post(tafAudioAppSem);
                break;
            case TAF_AUDIO_MEDIA_ERROR:
                LE_INFO("File event is TAF_AUDIO_MEDIA_ERROR.");
                break;
            case TAF_AUDIO_MEDIA_NO_MORE_SAMPLES:
                LE_INFO("File event is TAF_AUDIO_MEDIA_NO_MORE_SAMPLES.");
                break;
            default:
                LE_INFO("File event is %d", event);
                break;
        }
    }
    void TEST_INCALL_DOWNLINK_RECORDING(){
        taf_audio_ConnectorRef_t connRef = taf_audio_CreateConnector();
        recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_RX);
        //rxStreamRef sshould be the same stream reference created and connected for VOICE_CALL.
        res = taf_audio_Connect(connRef, rxStreamRef);
        if(res != LE_OK)
        {
            LE_ERROR("Failed to connect rxStreamRef to connRef");
        }
        res = taf_audio_Connect(connRef, recorderRef);
        if (res != LE_OK)
        {
            LE_ERROR("Failed to connect recorderRef to connRef");
        }
        Recorder_thread_ref = le_thread_Create("taf_audio_svc_test_thread",
                Test_taf_audio_AddHandler, (void*)recorderRef);
        le_thread_Start(Recorder_thread_ref);

        le_sem_Wait(tafAudioAppSem);

        res = taf_audio_RecordFile(recorderRef, recordfilePath);
        if (res != LE_OK)
        {
            LE_ERROR("Failed started the file recording");
        }

        //To stop the recording.
        res = taf_audio_Stop(recorderRef);
        if (res == LE_OK)
        {
            LE_ERROR("Failed stopped the file recording");
        }

        le_sem_Wait(tafAudioAppSem); // Wait till MEDIA_STOPPED is received.

        taf_audio_Disconnect(connRef, recorderRef);

        taf_audio_Close(recorderRef);

        taf_audio_RemoveMediaHandler(MediaHandlerRef);

        taf_audio_DeleteConnector(connRef);
    }
   @endverbatim
 *
 * @subsection c_open_route_playback Open route for local playback
 *
 * @verbatim
    taf_audio_StreamRef_t sinkRef = NULL, sourceRef = NULL, playerRef = NULL;
    taf_audio_RouteRef_t routeRef = NULL;
    const char* filePath = "/data/test.wav"
	le_sem_Ref_t tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);
    void* Test_taf_audio_AddHandler(void* ctxPtr)
    {
        le_sem_Ref_t sem = NULL;
        taf_audio_ConnectService();

        taf_audio_StreamRef_t streamRef = (taf_audio_StreamRef_t)ctxPtr;
        MediaHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
        if(MediaHandlerRef != NULL)
            LE_INFO("Successfully registered for media handler");

        sem = le_sem_FindSemaphore("tafAudioAppSem");
        if(sem != NULL)
        {
            le_sem_Post(sem);
        }
        else
        {
            LE_ERROR("tafAudioAppSem is NULL!");
        }
        le_event_RunLoop();
        return NULL;
    }
    static void MyMediaEventHandler
    (
        taf_audio_StreamRef_t          streamRef,
        taf_audio_MediaEvent_t         event,
        void*                               contextPtr
    )
    {
        switch(event)
        {
            case TAF_AUDIO_MEDIA_ENDED:
                LE_INFO(" Playback completed");
                break;
            case TAF_AUDIO_MEDIA_STOPPED:
                LE_INFO(" Playback/capture stopped");
                break;
            case TAF_AUDIO_MEDIA_ERROR:
                LE_INFO("File event is TAF_AUDIO_MEDIA_ERROR.");
                break;
            case TAF_AUDIO_MEDIA_NO_MORE_SAMPLES:
                LE_INFO("File event is TAF_AUDIO_MEDIA_NO_MORE_SAMPLES.");
                break;
            default:
                LE_INFO("File event is %d", event);
                break;
        }
    }
    void TEST_PLAYBACK()
    {
        routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_LOCAL_PLAYBACK,
                &sinkRef, &sourceRef);
        if(routeRef != NULL)
            LE_INFO("Successfully open route for playback");

        playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
        if(playerRef != NULL)
            LE_INFO("Successfully created player stream reference");

        Player_thread_ref = le_thread_Create("taf_audio_ut_thread",
                Test_taf_audio_AddHandler, (void*)playerRef);
        le_thread_Start(Player_thread_ref);
        le_sem_Wait(tafAudioAppSem);

        taf_audio_ConnectorRef_t playerConnRef = taf_audio_CreateConnector();
        if(playerConnRef != NULL)
            LE_INFO("Successfully created connector reference for playback");

        res = taf_audio_Connect(playerConnRef, sinkRef);
        if(res == LE_OK)
            LE_INFO("Successfully connected sinkRef to playerConnRef");

        res = taf_audio_Connect(playerConnRef, playerRef);
        if(res == LE_OK)
            LE_INFO("Successfully connected playerRef to playerConnRef");

        res = taf_audio_PlayFile(playerRef, filePath);
        if(res == LE_OK)
            LE_INFO("Successfully file playback has started");

        // To stop playback
        res = taf_audio_Stop(playerRef);
        if(res == LE_OK)
            LE_INFO("Successfully stopped the file playback");

        taf_audio_Disconnect(connRef, playerRef);

        taf_audio_Close(playerRef);

        res = taf_audio_CloseRoute(routeRef);
        if(res == LE_OK)
            LE_INFO("Successfully closed the playback route");
    }
   @endverbatim
 *
 * @subsection c_open_route_playback Open route for local playback of a playlist of audio files
 *
 * Plays the audio file in the local device selected. For PCM file if number of channels read from
 * the header is 2, channel type will be taken as both left and right. If number of channels read
 * from the header is 1, channel type will be taken as left. For, AMR by default it is taken as
 * channel type left.
 *
 * @verbatim
    taf_audio_StreamRef_t sinkRef = NULL, sourceRef = NULL, playerRef = NULL;
    taf_audio_RouteRef_t routeRef = NULL;
    le_sem_Ref_t tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);

    void* Test_taf_audio_AddHandler(void* ctxPtr)
    {
        le_sem_Ref_t sem = NULL;
        taf_audio_ConnectService();

        taf_audio_StreamRef_t streamRef = (taf_audio_StreamRef_t)ctxPtr;
        MediaHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
        if(MediaHandlerRef != NULL)
            LE_INFO("Successfully registered for media handler");

        sem = le_sem_FindSemaphore("tafAudioAppSem");
        if(sem != NULL)
        {
            le_sem_Post(sem);
        }
        else
        {
            LE_ERROR("tafAudioAppSem is NULL!");
        }
        le_event_RunLoop();
        return NULL;
    }
    static void MyMediaEventHandler
    (
        taf_audio_StreamRef_t          streamRef,
        taf_audio_MediaEvent_t         event,
        void*                               contextPtr
    )
    {
        switch(event)
        {
            case TAF_AUDIO_MEDIA_ENDED:
                LE_INFO(" Playback completed");
                break;
            case TAF_AUDIO_MEDIA_STOPPED:
                LE_INFO(" Playback/capture stopped");
                break;
            case TAF_AUDIO_MEDIA_ERROR:
                LE_INFO("File event is TAF_AUDIO_MEDIA_ERROR.");
                break;
            case TAF_AUDIO_MEDIA_NO_MORE_SAMPLES:
                LE_INFO("File event is TAF_AUDIO_MEDIA_NO_MORE_SAMPLES.");
                break;
            default:
                LE_INFO("File event is %d", event);
                break;
        }
    }
    void TEST_PLAYLIST_PLAYBACK()
    {
        routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_LOCAL_PLAYBACK,
                &sinkRef, &sourceRef);
        if(routeRef != NULL)
            LE_INFO("Successfully open route for playback");

        playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
        if(playerRef != NULL)
            LE_INFO("Successfully created player stream reference");

        Player_thread_ref = le_thread_Create("taf_audio_ut_thread",
                Test_taf_audio_AddHandler, (void*)playerRef);
        le_thread_Start(Player_thread_ref);
        le_sem_Wait(tafAudioAppSem);

        taf_audio_ConnectorRef_t playerConnRef = taf_audio_CreateConnector();
        if(playerConnRef != NULL)
            LE_INFO("Successfully created connector reference for playback");

        res = taf_audio_Connect(playerConnRef, sinkRef);
        if(res == LE_OK)
            LE_INFO("Successfully connected sinkRef to playerConnRef");

        res = taf_audio_Connect(playerConnRef, playerRef);
        if(res == LE_OK)
            LE_INFO("Successfully connected playerRef to playerConnRef");

        const char* srcPath = "/data/test.wav";
        int repeat = 1;
        taf_audio_PlayFileConfig_t playFileConfig[1] = {0};
        snprintf(playFileConfig[0].srcPath, sizeof(playFileConfig[0].srcPath), srcPath);
        playFileConfig[0].repeat = repeat;

        res = taf_audio_PlayFileList(playerRef, playFileConfig,
                sizeof(playFileConfig)/sizeof(taf_audio_PlayFileConfig_t));
        if(res == LE_OK)
            LE_INFO(res == LE_OK, "Successfully started the file list playback");

        // To stop playback
        res = taf_audio_Stop(playerRef);
        if(res == LE_OK)
            LE_INFO("Successfully stopped the file list playback");

        taf_audio_Disconnect(connRef, playerRef);

        taf_audio_Close(playerRef);

        res = taf_audio_CloseRoute(routeRef);
        if(res == LE_OK)
            LE_INFO("Successfully closed the playback route");
    }
   @endverbatim
 *
 * @subsection c_open_route_record Open route for local recording
 *
 * Records what ever is played near the local microphone device selected. File will be recorded in
 * PCM format and with the channel type both left and right.
 *
 * @verbatim
    taf_audio_StreamRef_t sinkRef = NULL, sourceRef = NULL, recorderRef = NULL;
    taf_audio_RouteRef_t routeRef = NULL;
    const char* filePath = "/data/record.wav"
    le_sem_Ref_t tafAudioAppSem = le_sem_Create("tafAudioAppSem", 0);

    void* Test_taf_audio_AddHandler(void* ctxPtr)
    {
        le_sem_Ref_t sem = NULL;
        taf_audio_ConnectService();

        taf_audio_StreamRef_t streamRef = (taf_audio_StreamRef_t)ctxPtr;
        MediaHandlerRef = taf_audio_AddMediaHandler(streamRef, MyMediaEventHandler, NULL);
        if(MediaHandlerRef != NULL)
            LE_INFO("Successfully registered for media handler");

        sem = le_sem_FindSemaphore("tafAudioAppSem");
        if(sem != NULL)
        {
            le_sem_Post(sem);
        }
        else
        {
            LE_ERROR("tafAudioAppSem is NULL!");
        }
        le_event_RunLoop();
        return NULL;
    }
    static void MyMediaEventHandler
    (
        taf_audio_StreamRef_t          streamRef,
        taf_audio_MediaEvent_t         event,
        void*                               contextPtr
    )
    {
        switch(event)
        {
            case TAF_AUDIO_MEDIA_ENDED:
                LE_INFO(" Playback completed");
                break;
            case TAF_AUDIO_MEDIA_STOPPED:
                LE_INFO(" Playback/capture stopped");
                break;
            case TAF_AUDIO_MEDIA_ERROR:
                LE_INFO("File event is TAF_AUDIO_MEDIA_ERROR.");
                break;
            case TAF_AUDIO_MEDIA_NO_MORE_SAMPLES:
                LE_INFO("File event is TAF_AUDIO_MEDIA_NO_MORE_SAMPLES.");
                break;
            default:
                LE_INFO("File event is %d", event);
                break;
        }
    }
    void TEST_RECORDING()
    {
        routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1,
                TAF_AUDIO_LOCAL_RECOIRDING, &sinkRef, &sourceRef);
        if(routeRef != NULL)
            LE_INFO("Successfully open route for recording");

        recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_TX);
        if(recorderRef != NULL)
            LE_INFO("Successfully created recorder stream reference");

        Recorder_thread_ref = le_thread_Create("taf_audio_ut_thread",
                Test_taf_audio_AddHandler, (void*)playerRef);
        le_thread_Start(Recorder_thread_ref);
        le_sem_Wait(tafAudioAppSem);

        taf_audio_ConnectorRef_t recorderConnRef = taf_audio_CreateConnector();
        if(recorderConnRef != NULL)
            LE_INFO("Successfully created connector reference for recording");

        res = taf_audio_Connect(recorderConnRef, sinkRef);
        if(res == LE_OK)
            LE_INFO("Successfully connected sinkRef to recorderConnRef");

        res = taf_audio_Connect(recorderConnRef, recorderRef);
        if(res == LE_OK)
            LE_INFO("Successfully connected playerRef to recorderConnRef");

        res = taf_audio_RecordFile(recorderRef, filePath);
        if(res == LE_OK)
            LE_INFO("Successfully file playback has started");

        // To stop recording
        res = taf_audio_Stop(recorderRef);
        if(res == LE_OK)
            LE_INFO("Successfully stopped the file recording");

        taf_audio_Disconnect(connRef, recorderRef);

        taf_audio_Close(recorderRef);

        res = taf_audio_CloseRoute(routeRef);
        if(res == LE_OK)
            LE_INFO("Successfully closed the recording route");
    }

   @endverbatim
 *
 * @section c_taf_audio_set_get_mute Manage the mute status
 * TelAF Audio Service helps to set/get mute status for modem RX/TX, player and recorder.
 *
 * @subsection c_set_get_mute_voice_call Set/get the mute status of Modem RX/TX during voice call
 *
 * @verbatim

    taf_audio_StreamRef_t rxStreamRef = NULL, txStreamRef = NULL;
    bool isMute;
    le_result_t res;

    rxStreamRef = taf_audio_OpenModemVoiceRx(1);

    txStreamRef = taf_audio_OpenModemVoiceTx(1, true);

    res = taf_audio_SetMute(rxStreamRef, true);
    if (res == LE_OK) {
        LE_INFO("Successfully muted modem RX");
    }

    res = taf_audio_GetMute(rxStreamRef, &isMute);
    if (res == LE_OK) {
        LE_INFO("Successfully get the mute status of modem RX as %s", isMute ? false : true);
    }

    res = taf_audio_SetMute(txStreamRef, true);
    if (res == LE_OK) {
        LE_INFO("Successfully muted modem TX");
    }

    res = taf_audio_GetMute(txStreamRef, &isMute);
    if (res == LE_OK) {
        LE_INFO("Successfully get the mute status of modem TX as %s", isMute ? false : true);
    }
   @endverbatim
 *
 * @subsection c_set_get_mute_player Set/get the mute status of player
 *
 * @verbatim

    taf_audio_StreamRef_t  = playerRef;
    bool isMute;
    le_result_t res;

    playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);

    res = taf_audio_SetMute(playerRef, true);
    if (res == LE_OK) {
        LE_INFO("Successfully playerRef is muted");
    }

    res = taf_audio_GetMute(playerRef, &isMute);
    if (res == LE_OK) {
        LE_INFO("Successfully get the mute status of player as %s", isMute ? false : true);
    }

   @endverbatim
 *
 * @subsection c_set_get_mute_recorder Set/get the mute status of recorder
 *
 * @verbatim

    taf_audio_StreamRef_t  = recorderRef;
    bool isMute;
    le_result_t res;

    recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_TX);

    res = taf_audio_SetMute(recorderRef, true);
    if (res == LE_OK) {
        LE_INFO("Successfully recorderRef is muted");
    }

    res = taf_audio_GetMute(recorderRef, &isMute);
    if (res == LE_OK) {
        LE_INFO("Successfully get the mute status of recorder as %s", isMute ? false : true);
    }

   @endverbatim
 *
 * @section c_taf_audio_set_get_volume Manage the volume levels
 * TelAF Audio Service helps to set/get volume level for modem RX, player and recorder.
 * Default volume levels for all the streams will be 0, and the application needs to set the
 * required volume using SetVolume.
 *
 * @subsection c_set_get_vol_voice_call Set/get the volume level of Modem RX
 *
 * @verbatim

    taf_audio_StreamRef_t rxStreamRef = NULL, txStreamRef = NULL;
    le_result_t res;
    double volLevel = 0.5;
    double getVolLevel;
    rxStreamRef = taf_audio_OpenModemVoiceRx(1);

    txStreamRef = taf_audio_OpenModemVoiceTx(1, true);

    res = taf_audio_SetVolume(rxStreamRef, volLevel);
    if (res == LE_OK) {
        LE_INFO("Successfully set the volume for modem RX");
    }

    res = taf_audio_GetVolume(rxStreamRef, &getVolLevel);
    if (res == LE_OK) {
        LE_INFO("Successfully get the volume level of modem RX as %f", getVolLevel);
    }

   @endverbatim
 *
 * @subsection c_set_get_vol_player Set/get the volume level of player
 *
 * @verbatim

    taf_audio_StreamRef_t  = playerRef;
    double volLevel = 0.5;
    double getVolLevel;
    le_result_t res;

    playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);

    res = taf_audio_SetVolume(playerRef, volLevel);
    if (res == LE_OK) {
        LE_INFO("Successfully set the volume level to playerRef");
    }

    res = taf_audio_GetVolume(playerRef, &getVolLevel);
    if (res == LE_OK) {
        LE_INFO("Successfully get the volume level of player as %f", getVolLevel);
    }

   @endverbatim
 *
 * @subsection c_set_get_vol_recorder Set/get the volume level of recorder
 *
 * @verbatim

    taf_audio_StreamRef_t  = recorderRef;
    double volLevel = 0.5;
    double getVolLevel;
    le_result_t res;

    recorderRef = taf_audio_OpenRecorder(TAF_AUDIO_TX);

    res = taf_audio_SetVolume(recorderRef, volLevel);
    if (res == LE_OK) {
        LE_INFO("Successfully set the volume level to recorderRef");
    }

    res = taf_audio_GetVolume(recorderRef, &getVolLevel);
    if (res == LE_OK) {
        LE_INFO("Successfully get the volume level of recorder as %f", getVolLevel);
    }

   @endverbatim
 *
 * @subsection c_play_stop_DTMF Play/Stop DTMF for voice Rx path.
 *
 * Supports DTMF generation in Rx path [On Near End Device]. Prerequisite for DTMF generation is
 * an active voice call.
 *
 * @verbatim

    static const char*  DtmfString = "5";
    static uint16_t     Duration = 5;
    static uint32_t     Pause = 1;
    static double       gain = 1.0;
    le_result_t res;

    taf_audio_StreamRef_t sinkRef = NULL;
    taf_audio_StreamRef_t sourceRef = NULL;
    taf_audio_RouteRef_t routeRef = NULL;
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    if(routeRef != NULL) {
        LE_INFO("OpenRoute successfull sinkRef %p sourceRef %p", sinkRef, sourceRef);
    }

    taf_audio_ConnectorRef_t rxConn = taf_audio_CreateConnector();
    taf_audio_ConnectorRef_t txConn = taf_audio_CreateConnector();

    taf_audio_StreamRef_t rxStreamRef = taf_audio_OpenModemVoiceRx(1);
    // true to enable ECNR configuration.
    taf_audio_StreamRef_t txStreamRef = taf_audio_OpenModemVoiceTx(1, true);

    res = taf_audio_Connect(rxConn, sinkRef);
    if(res == LE_OK) {
        LE_INFO("sinkRef connected successfully to rxConn" );
    }

    res = taf_audio_Connect(txConn, sourceRef);
    if(res == LE_OK) {
        LE_INFO("sourceRef connected successfully to txConn" );
    }

    res = taf_audio_Connect(rxConn, rxStreamRef);
    if(res == LE_OK) {
        LE_INFO("rxStreamRef connected successfully to rxConn" );
    }

    res = taf_audio_Connect(txConn, txStreamRef);
    if(res == LE_OK) {
        LE_INFO("txStreamRef connected successfully to txConn" );
    }

    LE_INFO("To test taf_audio_PlayDtmf on rxStreamRef.%p", rxStreamRef);
    res = taf_audio_PlayDtmf(rxStreamRef, DtmfString, Duration, Pause, gain);
    if(res == LE_OK) {
        LE_INFO("taf_audio_PlayDtmf - Pass");
    }

    LE_INFO("To test taf_audio_StopDtmf on rxStreamRef.%p", rxStreamRef);
    res = taf_audio_StopDtmf(rxStreamRef);
    if(res == LE_OK) {
        LE_INFO("taf_audio_StopDtmf - Pass");
    }

    LE_INFO("To test taf_audio_PlaySignallingDtmf");
    res = taf_audio_PlaySignallingDtmf(slotId, dtmfPtr, duration, pause);
    if(res == LE_OK) {
        LE_INFO("taf_audio_PlaySignallingDtmf - Pass");
    }

    LE_INFO("To test Test_Audio_Stop_Signalling_Dtmf");
    res = taf_audio_StopSignallingDtmf(slotId);
    if(res == LE_OK) {
        LE_INFO("Test_Audio_Stop_Signalling_Dtmf - Pass");
    }

    taf_audio_Disconnect(txConn, txStreamRef);

    taf_audio_Disconnect(rxConn, sinkRef);

    taf_audio_Disconnect(txConn, sourceRef);

    taf_audio_Disconnect(rxConn, rxStreamRef);

    res = taf_audio_CloseRoute(routeRef);
    if(res == LE_OK) {
        LE_INFO("taf_audio_CloseRoute successful" );
    }

   @endverbatim
 *
 **
 * @subsection c_DTMF_Detection DTMF detection for voice Rx path.
 *
 * Supports DTMF detection in Rx path [Detection On Far End Voice Samples]. Prerequisite for DTMF
 * detection is an active voice call.
 *
 * @verbatim

    le_result_t res;
    static taf_audio_DtmfDetectorHandlerRef_t dtmfDetectHandlerRef = NULL;

    static void MyDtmfDetectorHandler
    (
        taf_audio_StreamRef_t streamRef,
        char  dtmf,
        void* contextPtr
    )
    {
        LE_INFO("MyDtmfDetectorHandler detects %c", dtmf);
        std::cout << "Dtmf tone detected for " << dtmf << std::endl;
    }

    taf_audio_StreamRef_t sinkRef = NULL;
    taf_audio_StreamRef_t sourceRef = NULL;
    taf_audio_RouteRef_t routeRef = NULL;
    routeRef = taf_audio_OpenRoute( TAF_AUDIO_ROUTE_1, TAF_AUDIO_VOICE_CALL,
            &sinkRef, &sourceRef);
    if(routeRef != NULL) {
        LE_INFO("OpenRoute successfull sinkRef %p sourceRef %p", sinkRef, sourceRef);
    }

    taf_audio_ConnectorRef_t rxConn = taf_audio_CreateConnector();
    taf_audio_ConnectorRef_t txConn = taf_audio_CreateConnector();

    taf_audio_StreamRef_t rxStreamRef = taf_audio_OpenModemVoiceRx(1);
    // true to enable ECNR configuration.
    taf_audio_StreamRef_t txStreamRef = taf_audio_OpenModemVoiceTx(1, true);

    res = taf_audio_Connect(rxConn, sinkRef);
    if(res == LE_OK) {
        LE_INFO("sinkRef connected successfully to rxConn" );
    }

    res = taf_audio_Connect(txConn, sourceRef);
    if(res == LE_OK) {
        LE_INFO("sourceRef connected successfully to txConn" );
    }

    res = taf_audio_Connect(rxConn, rxStreamRef);
    if(res == LE_OK) {
        LE_INFO("rxStreamRef connected successfully to rxConn" );
    }

    res = taf_audio_Connect(txConn, txStreamRef);
    if(res == LE_OK) {
        LE_INFO("txStreamRef connected successfully to txConn" );
    }

    //Add the handler for DTMF detection
    dtmfDetectHandlerRef = taf_audio_AddDtmfDetectorHandler(rxStreamRef, MyDtmfDetectorHandler,
            NULL);

    //Remove the handler for DTMF detection
    taf_audio_RemoveDtmfDetectorHandler(dtmfDetectHandlerRef);

    taf_audio_Disconnect(txConn, txStreamRef);

    taf_audio_Disconnect(rxConn, sinkRef);

    taf_audio_Disconnect(txConn, sourceRef);

    taf_audio_Disconnect(rxConn, rxStreamRef);

    res = taf_audio_CloseRoute(routeRef);
    if(res == LE_OK) {
        LE_INFO("taf_audio_CloseRoute successful" );
    }

   @endverbatim
 *
 *
 */


#include "legato.h"
#include "interfaces.h"


typedef struct Object_ {
    void * self;
} Object_t;

static le_mem_PoolRef_t AudioObjectPool;
static le_ref_MapRef_t AudioRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Creates a new connector.
 *
 * @return
 *    - Reference to the audio connector on success.
 *    - NULL on error.
 */
//--------------------------------------------------------------------------------------------------
taf_audio_ConnectorRef_t taf_audio_CreateConnector
(
    void
)
{
    Object_t * obj = (Object_t *) le_mem_ForceAlloc(AudioObjectPool);
    return (taf_audio_ConnectorRef_t )le_ref_CreateRef(AudioRefMap, obj);
}
//--------------------------------------------------------------------------------------------------
/**
 * Deletes a connected connector.
 *
 * @note Function fails on passing bad reference.
 */
//--------------------------------------------------------------------------------------------------
void taf_audio_DeleteConnector
(
    taf_audio_ConnectorRef_t connRef
        ///< [IN] Connector reference.
)
{
    LE_INFO("[simulation]: delete connector on %p", connRef);

    Object_t* obj = (Object_t *) le_ref_Lookup(AudioRefMap, connRef);
    LE_ASSERT(obj != NULL);
    le_mem_Release(obj);
    le_ref_DeleteRef(AudioRefMap, connRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Closes the stream reference.
 *
 * @note Function fails on passing bad reference.
 */
//--------------------------------------------------------------------------------------------------
void taf_audio_Close
(
    taf_audio_StreamRef_t sRef
        ///< [IN] Audio stream reference.
)
{
    LE_INFO("[simulation]: close stream on %p", sRef);

    Object_t* obj = (Object_t *) le_ref_Lookup(AudioRefMap, sRef);
    LE_ASSERT(obj != NULL);
    le_mem_Release(obj);
    le_ref_DeleteRef(AudioRefMap, sRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Connects a connector for an audio stream reference.
 *
 * @return
 *     - LE_OK            Success.
 *     - LE_BAD_PARAMETER On bad references.
 *     - LE_FAULT         Failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_Connect
(
    taf_audio_ConnectorRef_t connRef,
        ///< [IN] Connector reference.
    taf_audio_StreamRef_t sRef
        ///< [IN] Audio stream reference.
)
{
    LE_INFO("[simulation]: %p .. connect .. %p", connRef, sRef);
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Disconnects a connector for an audio stream reference.
 *
 * @note Function fails on passing bad reference.
 */
//--------------------------------------------------------------------------------------------------
void taf_audio_Disconnect
(
    taf_audio_ConnectorRef_t connRef,
        ///< [IN] Connector reference.
    taf_audio_StreamRef_t sRef
        ///< [IN] Audio stream reference.
)
{
    LE_INFO("[simulation]: disconnect on [%p .. %p]", connRef, sRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Opens an audio stream reference for voice Rx.
 *
 * @return
 *    - Reference to an audio stream
 *    - NULL on error.
 */
//--------------------------------------------------------------------------------------------------
taf_audio_StreamRef_t taf_audio_OpenModemVoiceRx
(
    uint32_t slotId
        ///< [IN] Slot ID.
)
{
    Object_t * obj = (Object_t *) le_mem_ForceAlloc(AudioObjectPool);
    return (taf_audio_StreamRef_t )le_ref_CreateRef(AudioRefMap, obj);
}
//--------------------------------------------------------------------------------------------------
/**
 * Opens an audio stream reference for voice Tx.
 *
 * @return
 *    - Reference to an audio stream
 *    - NULL on error.
 */
//--------------------------------------------------------------------------------------------------
taf_audio_StreamRef_t taf_audio_OpenModemVoiceTx
(
    uint32_t slotId,
        ///< [IN] Slot ID.
    bool enableEcnr
        ///< [IN] Enable ECNR.
)
{
    Object_t * obj = (Object_t *) le_mem_ForceAlloc(AudioObjectPool);
    return (taf_audio_StreamRef_t )le_ref_CreateRef(AudioRefMap, obj);
}

//--------------------------------------------------------------------------------------------------
/**
 * Opens the audio stream for the requested route and mode.
 * If the audio VHAL module is available, the VHAL OpenRoute will get called after the requested
 * audio route for the mode is established.
 *
 * Reference created is used to close the route using CloseRoute.
 *
 * @return
 *     - Route reference on success.
 *     - NULL on failure.
 */
//--------------------------------------------------------------------------------------------------
taf_audio_RouteRef_t taf_audio_OpenRoute
(
    taf_audio_RouteId_t route,
        ///< [IN] Route ID.
    taf_audio_Mode_t mode,
        ///< [IN] Mode type.
    taf_audio_StreamRef_t* sinkRefPtr,
        ///< [OUT] Sink stream reference.
    taf_audio_StreamRef_t* sourceRefPtr
        ///< [OUT] Source stream reference.
)
{
    LE_ASSERT(sinkRefPtr != NULL);
    LE_ASSERT(sourceRefPtr != NULL);

    Object_t * InputStream = (Object_t *) le_mem_ForceAlloc(AudioObjectPool);
    Object_t * OutputStream = (Object_t *) le_mem_ForceAlloc(AudioObjectPool);
    Object_t * RouteObj = (Object_t *) le_mem_ForceAlloc(AudioObjectPool);

    *sinkRefPtr = (taf_audio_StreamRef_t)le_ref_CreateRef(AudioRefMap, InputStream);
    *sourceRefPtr = (taf_audio_StreamRef_t)le_ref_CreateRef(AudioRefMap, OutputStream);;

    return (taf_audio_RouteRef_t)le_ref_CreateRef(AudioRefMap, RouteObj);
}
//--------------------------------------------------------------------------------------------------
/**
 * Closes the route reference if opened.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_BAD_PARAMETER      Route reference is not opened or invalid reference.
 *     - LE_FAULT              Failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_CloseRoute
(
    taf_audio_RouteRef_t routeRef
        ///< [IN] Route reference created on OpenRoute.
)
{
    LE_INFO("[simulation]: close route on %p", routeRef);

    Object_t* obj = (Object_t *) le_ref_Lookup(AudioRefMap, routeRef);
    LE_ASSERT(obj != NULL);
    le_mem_Release(obj);
    le_ref_DeleteRef(AudioRefMap, routeRef);

    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Creates the player stream for the direction.
 *
 * @return
 *     - Stream reference on success.
 *     - NULL on failure.
 */
//--------------------------------------------------------------------------------------------------
taf_audio_StreamRef_t taf_audio_OpenPlayer
(
    taf_audio_Direction_t direction
        ///< [IN] Rx for local playback and Tx for remote playback.
)
{
    Object_t * obj = (Object_t *)le_mem_ForceAlloc(AudioObjectPool);
    return (taf_audio_StreamRef_t )le_ref_CreateRef(AudioRefMap, obj);
}
//--------------------------------------------------------------------------------------------------
/**
 * Plays the audio file.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_BAD_PARAMETER      Stream reference or file path is not valid.
 *     - LE_FAULT              Failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_PlayFile
(
    taf_audio_StreamRef_t streamRef,
        ///< [IN] Player stream reference created on OpenPlayer.
    const char* LE_NONNULL scrPath
        ///< [IN] The absolute audio file path.
)
{
    LE_INFO("[simulation] --> Playing [%s] on [%p] ..", scrPath, streamRef);
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Creates the recorder stream for the direction.
 *
 * @return
 *     - Stream reference on success.
 *     - NULL on failure.
 */
//--------------------------------------------------------------------------------------------------
taf_audio_StreamRef_t taf_audio_OpenRecorder
(
    taf_audio_Direction_t direction
        ///< [IN] Tx for local recorder and Rx for remote recorder.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Records the audio file in wav format.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_BAD_PARAMETER      Stream reference or file path is not valid.
 *     - LE_FAULT              Failure.
 * @note Recording will be stopped automatically by the tafAudioSvc, when the recorded file size
 * reaches the maxFileBytes defined in service adef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_RecordFile
(
    taf_audio_StreamRef_t streamRef,
        ///< [IN] Recoder stream reference created on OpenRecorder.
    const char* LE_NONNULL scrPath
        ///< [IN] The absolute audio file path.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Stops the active playback/capture.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_BAD_PARAMETER      Invalid stream reference.
 *     - LE_FAULT              Failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_Stop
(
    taf_audio_StreamRef_t streamRef
        ///< [IN] Player/Recoder stream reference created on OpenPlayer/OpenRecorder.
)
{
    LE_INFO("[simulation]: stop player [%p]", streamRef);
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_audio_Media'
 *
 * This event provides information on player/recorder stream events.
 */
//--------------------------------------------------------------------------------------------------
taf_audio_MediaHandlerRef_t taf_audio_AddMediaHandler
(
    taf_audio_StreamRef_t streamRef,
        ///< [IN] Audio stream reference.
    taf_audio_MediaHandlerFunc_t handlerPtr,
        ///< [IN] Media handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_INFO("[simulation] register handler for [AddMedia] on %p", streamRef);
    Object_t * obj = (Object_t *) le_mem_ForceAlloc(AudioObjectPool);
    return (taf_audio_MediaHandlerRef_t)le_ref_CreateRef(AudioRefMap, obj);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_audio_Media'
 */
//--------------------------------------------------------------------------------------------------
void taf_audio_RemoveMediaHandler
(
    taf_audio_MediaHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_INFO("[simulation]: remove handler: %p", handlerRef);
    Object_t * obj = (Object_t *) le_ref_Lookup(AudioRefMap, handlerRef);
    LE_ASSERT(obj != NULL);
    le_mem_Release(obj);
    le_ref_DeleteRef(AudioRefMap, handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Plays the audio files in the play list.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_BUSY               Another playback is active.
 *     - LE_BAD_PARAMETER      Stream or playlist reference is not valid.
 *     - LE_FAULT              Failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_PlayFileList
(
    taf_audio_StreamRef_t streamRef,
        ///< [IN] Audio stream reference.
    const taf_audio_PlayFileConfig_t* fileConfigPtr,
        ///< [IN] Array of PlayFileConfig.
    size_t fileConfigSize
        ///< [IN]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the mute status to the modem Rx/Tx, player and recorder stream reference.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_UNSUPPORTED        Given stream is not supported. Applicable for remote player
 *                             stream (with Tx direction) and remote recorder
 *                             stream (with Rx direction).
 *     - LE_BAD_PARAMETER      Invalid stream reference.
 *     - LE_FAULT              Failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_SetMute
(
    taf_audio_StreamRef_t streamRef,
        ///< [IN] Audio stream reference.
    bool isMute
        ///< [IN] True to mute, false to unmute.
)
{
    LE_INFO("[simulation]: set mute [%d] on [%p]", isMute, streamRef);
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the mute status of modem Rx/Tx, player and recorder stream reference.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_UNSUPPORTED        Given stream is not supported. Applicable for remote player
 *                             stream (with Tx direction) and remote recorder
 *                             stream (with Rx direction).
 *     - LE_BAD_PARAMETER      Invalid stream reference.
 *     - LE_FAULT              Failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_GetMute
(
    taf_audio_StreamRef_t streamRef,
        ///< [IN] Audio stream reference.
    bool* isMutePtr
        ///< [OUT] Mute status.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the volume to the modem Rx, player and recorder stream reference.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_UNSUPPORTED        Given stream is not supported. Applicable for remote player
 *                             stream (with Tx direction) and remote recorder
 *                             stream (with Rx direction).
 *     - LE_BAD_PARAMETER      Invalid stream reference.
 *     - LE_FAULT              Failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_SetVolume
(
    taf_audio_StreamRef_t streamRef,
        ///< [IN] Audio stream reference.
    double volumeLevel
        ///< [IN] Volume level.
)
{
    LE_INFO("[simulation]: set volume level: %f", volumeLevel);
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the volume of modem Rx, player and recorder stream reference.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_UNSUPPORTED        Given stream is not supported. Applicable for remote player
 *                             stream (with Tx direction) and remote recorder
 *                             stream (with Rx direction).
 *     - LE_BAD_PARAMETER      Invalid stream reference.
 *     - LE_FAULT              Failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_GetVolume
(
    taf_audio_StreamRef_t streamRef,
        ///< [IN] Audio stream reference.
    double* volumeLevelPtr
        ///< [OUT] Volume level.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Starts DTMF playback on voice TX path.
 * This function should be called to request the Mobile Network to generate
 * DTMF tones on the remote audio end.
 * @b NOTE: This API only supports 12 chars (0 to 9, * and #).
 * @return
 *     - LE_OK                 Success.
 *     - LE_BAD_PARAMETER      Invalid parameters.
 *     - LE_BUSY               A DTMF playback is in progress.
 *     - LE_FAULT              Failure.
 *     - LE_UNSUPPORTED        No active RF call
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_PlaySignallingDtmf
(
    uint32_t slotId,
        ///< [IN] Slot ID.
    const char* LE_NONNULL dtmf,
        ///< [IN] DTMFs to play.
    uint32_t duration,
        ///< [IN] DTMF duration in milliseconds.
    uint32_t pause
        ///< [IN] Pause duration between tones in milliseconds.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Starts DTMF playback on voice RX path.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_BAD_PARAMETER      Invalid stream reference.
 *     - LE_BUSY               A DTMF playback is in progress.
 *     - LE_FAULT              Failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_PlayDtmf
(
    taf_audio_StreamRef_t streamRef,
        ///< [IN] Modem Rx stream reference.
    const char* LE_NONNULL dtmf,
        ///< [IN] DTMFs to play.
    uint16_t duration,
        ///< [IN] DTMF duration in milliseconds, for infinity tone
    uint32_t pause,
        ///< [IN] Pause duration between tones in milliseconds.
    double gain
        ///< [IN] Range 0.0 - 1.0.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Stops the DTMF playback on voice RX path.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_BAD_PARAMETER      Invalid stream reference.
 *     - LE_FAULT              Failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_StopDtmf
(
    taf_audio_StreamRef_t streamRef
        ///< [IN] Modem Rx stream reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Stops the DTMF playback on voice TX path.
 *
 * @return
 *     - LE_OK                 Success.
 *     - LE_BAD_PARAMETER      Invalid slotId.
 *     - LE_FAULT              Failure.
 *     - LE_UNSUPPORTED        No active RF call
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_audio_StopSignallingDtmf
(
    uint32_t slotId
        ///< [IN] Slot ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_audio_DtmfDetector'
 *
 * This event provides notification on DTMF tone detection for the registered player reference.
 */
//--------------------------------------------------------------------------------------------------
taf_audio_DtmfDetectorHandlerRef_t taf_audio_AddDtmfDetectorHandler
(
    taf_audio_StreamRef_t streamRef,
        ///< [IN] Modem Rx stream reference.
    taf_audio_DtmfDetectorHandlerFunc_t handlerPtr,
        ///< [IN] DTMF handler.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_audio_DtmfDetector'
 */
//--------------------------------------------------------------------------------------------------
void taf_audio_RemoveDtmfDetectorHandler
(
    taf_audio_DtmfDetectorHandlerRef_t handlerRef
        ///< [IN]
)
{
}

COMPONENT_INIT
{
    AudioObjectPool = le_mem_CreatePool("AudioObjectPool", sizeof(Object_t));

    AudioRefMap = le_ref_CreateMap("AudioRefMap", 5);

    LE_INFO("[simulation] audio svc stub code init.");
}
