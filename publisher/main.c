/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * .. _pubsub-tutorial:
 *
 * Working with Publish/Subscribe
 * ------------------------------
 *
 * Work in progress: This Tutorial will be continuously extended during the next
 * PubSub batches. More details about the PubSub extension and corresponding
 * open62541 API are located here: :ref:`pubsub`.
 *
 * Publishing Fields
 * ^^^^^^^^^^^^^^^^^
 * The PubSub publish example demonstrates the simplest way to publish
 * information from the information model over UDP multicast using the UADP
 * encoding.
 *
 * **Connection handling**
 *
 * PubSubConnections can be created and deleted on runtime. More details about
 * the system preconfiguration and connection can be found in
 * ``tutorial_pubsub_connection.c``.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/types.h>

#include <stdio.h>
#include <stdlib.h>

#include <glib-2.0/glib.h>
#include <time.h>
#include <sys/time.h>

UA_NodeId connectionIdent, publishedDataSetIdent, writerGroupIdent,
          dataSetWriterIdent;
GHashTable *g_ping;
UA_Int64 g_ping_sequence_nr;

int64_t GetTimeStamp(void);

int64_t GetTimeStamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

struct ping_data
{
    int64_t sequence_number;
    int64_t send_time;
    int64_t recv_time;
};

static void
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl){
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    /* Changed to static publisherId from random generation to identify
     * the publisher on Subscriber side */
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = 2234;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

/**
 * **PublishedDataSet handling**
 *
 * The PublishedDataSet (PDS) and PubSubConnection are the toplevel entities and
 * can exist alone. The PDS contains the collection of the published fields. All
 * other PubSub elements are directly or indirectly linked with the PDS or
 * connection. */
static void
addPublishedDataSet(UA_Server *server) {
    /* The PublishedDataSetConfig contains all necessary public
    * information for the creation of a new PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
}

/**
 * **DataSetField handling**
 *
 * The DataSetField (DSF) is part of the PDS and describes exactly one published
 * field. */
static void
addDataSetField(UA_Server *server) {
    /* Add a field to the previous created PublishedDataSet */
    {
        UA_NodeId dataSetFieldIdent;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
        dataSetFieldConfig.field.variable.promotedField = false;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
            UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME);
        dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_DataSetFieldResult result;
        result = UA_Server_addDataSetField(server, publishedDataSetIdent,
                                  &dataSetFieldConfig, &dataSetFieldIdent);

        if (result.result != UA_STATUSCODE_GOOD) {
            printf("Failed to add data set field!!!!!\n");
        }
    }

    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("An Int32");
    dataSetFieldConfig.field.variable.promotedField = false;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
        UA_NODEID_STRING(1, "ping");
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataSetFieldResult result;
    result = UA_Server_addDataSetField(server, publishedDataSetIdent,
                              &dataSetFieldConfig, &dataSetFieldIdent);
    if (result.result != UA_STATUSCODE_GOOD) {
        printf("Failed to add data set field cInt32!!!!!\n");
    }

}

/**
 * **WriterGroup handling**
 *
 * The WriterGroup (WG) is part of the connection and contains the primary
 * configuration parameters for the message creation. */
static void
addWriterGroup(UA_Server *server) {
    /* Now we create a new WriterGroupConfig and add the group to the existing
     * PubSubConnection. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;

    /* Change message settings of writerGroup to send PublisherId,
     * WriterGroupId in GroupHeader and DataSetWriterId in PayloadHeader
     * of NetworkMessage */
    UA_UadpWriterGroupMessageDataType writerGroupMessage;
    UA_UadpWriterGroupMessageDataType_init(&writerGroupMessage);
    writerGroupMessage.networkMessageContentMask =
        (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                           UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                           UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                           UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);

    /* The configuration flags for the messages are encapsulated inside the
     * message- and transport settings extension objects. These extension
     * objects are defined by the standard. e.g.
     * UadpWriterGroupMessageDataType */
    UA_ExtensionObject_setValue(&writerGroupConfig.messageSettings, &writerGroupMessage,
                                &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);

    UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig, &writerGroupIdent);
}

/**
 * **DataSetWriter handling**
 *
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional information for the
 * message generation. */
static void
addDataSetWriter(UA_Server *server) {
    /* We need now a DataSetWriter within the WriterGroup. This means we must
     * create a new DataSetWriterConfig and add call the addWriterGroup function. */
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 0;
    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);
}

static void
addVariable(UA_Server *server)
{
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int64 myInteger = 0;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT64]);
    attr.description = UA_LOCALIZEDTEXT("en-US","ping");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","ping");
    attr.dataType = UA_TYPES[UA_TYPES_INT64].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "ping");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "ping");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

static void
updatePingSequenceCallback(UA_Server *server,
               void *data) {

    UA_NodeId node_id = UA_NODEID_STRING(1, "ping");

    g_ping_sequence_nr = g_ping_sequence_nr + 1;

    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "The value is: %i", g_ping_sequence_nr);

    struct ping_data *pingData = (struct ping_data*)malloc(sizeof(struct ping_data));
    pingData->sequence_number = g_ping_sequence_nr;
    pingData->recv_time = 0;
    pingData->send_time = 0;

    g_hash_table_insert(g_ping, &g_ping_sequence_nr, &pingData);

    UA_Variant updatedValue;
    UA_Variant_init(&updatedValue);
    UA_Variant_setScalar(&updatedValue, &g_ping_sequence_nr, &UA_TYPES[UA_TYPES_INT64]);
    UA_Server_writeValue(server, node_id, updatedValue);
}

static void
addCallbackIncrement(UA_Server *server)
{
    UA_Server_addRepeatedCallback(server, updatePingSequenceCallback, NULL, 1000, NULL);
}

static void
logPingReadCallback(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeId, void *nodeContext,
               const UA_NumericRange *range, const UA_DataValue *data)
{
    struct ping_data *pingData = (struct  ping_data*)g_hash_table_lookup(g_ping, &g_ping_sequence_nr);
    if (pingData != NULL)
    {
        if (pingData->send_time == 0)
        {
            pingData->send_time = GetTimeStamp();
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Added timestamp on: %d, %ld", pingData->sequence_number, pingData->send_time);
        }

    }


    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "The ping value was read");
}

static void
addLogPingReadCallback(UA_Server *server)
{
    UA_NodeId node_id = UA_NODEID_STRING(1, "ping");
    UA_ValueCallback callback;
    callback.onRead = logPingReadCallback;
    UA_Server_setVariableNode_valueCallback(server, node_id, callback);
}


/**
 * That's it! You're now publishing the selected fields. Open a packet
 * inspection tool of trust e.g. wireshark and take a look on the outgoing
 * packages. The following graphic figures out the packages created by this
 * tutorial.
 *
 * .. figure:: ua-wireshark-pubsub.png
 *     :figwidth: 100 %
 *     :alt: OPC UA PubSub communication in wireshark
 *
 * The open62541 subscriber API will be released later. If you want to process
 * the the datagrams, take a look on the ``ua_network_pubsub_networkmessage.c``
 * which already contains the decoding code for UADP messages.
 *
 * It follows the main server code, making use of the above definitions. */

static int
run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl) {
    /* Create a server */
    UA_Server *server = UA_Server_new();
    g_ping_sequence_nr = 0;
    g_ping = g_hash_table_new(g_int64_hash, g_int64_equal);

    /* Add the PubSub components. They are initially disabled */
    addPubSubConnection(server, transportProfile, networkAddressUrl);
    addVariable(server);
    addCallbackIncrement(server);
    addLogPingReadCallback(server);
    addPublishedDataSet(server);
    addDataSetField(server);
    addWriterGroup(server);
    addDataSetWriter(server);

    /* Enable the PubSubComponents */
    UA_Server_enableAllPubSubComponents(server);

    /* Run the server */
    UA_StatusCode retval = UA_Server_runUntilInterrupt(server);

    /* Delete the server */
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}

int main(int argc, char **argv) {
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile =
                UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if (argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else {
            printf("Error: unknown URI\n");
            return EXIT_FAILURE;
        }
    }
    if (argc > 2) {
        networkAddressUrl.networkInterface = UA_STRING(argv[2]);
    }

    return run(&transportProfile, &networkAddressUrl);
}
