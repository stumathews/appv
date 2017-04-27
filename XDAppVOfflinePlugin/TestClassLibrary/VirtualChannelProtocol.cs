using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Linq;
using System.Text;

namespace TestClassLibrary
{
  /// <summary>
    /// Request object used to communicate from ther plugin to the service
    /// </summary>
    [Serializable]
    public class VirtualChannelRequest
    {
        /// <summary>
        /// The requested task
        /// </summary>
        [SuppressMessage("Microsoft.Design", "CA1051:DoNotDeclareVisibleInstanceFields", Justification = "This is to be a flat object with no function as its a protocol object")]
        public VirtualChannelRequestTask RequestTask;
        /// <summary>
        /// The message body
        /// </summary>
        [SuppressMessage("Microsoft.Design", "CA1051:DoNotDeclareVisibleInstanceFields", Justification = "This is to be a flat object with no function as its a protocol object")]
        public string MessageBody;
        public VirtualChannelRequest() {}
    }

    /// <summary>
    /// Response object used to communicate response information from the serivce to the plugin
    /// </summary>
    [Serializable]
    public class VirtualChannelResponse
    {
        /// <summary>
        /// The response code
        /// </summary>
        [SuppressMessage("Microsoft.Design", "CA1051:DoNotDeclareVisibleInstanceFields", Justification = "This is to be a flat object with no function as its a protocol object")]
        public VirtualChannelResponseCode ResponseCode;
        /// <summary>
        /// The message body
        /// </summary>
        [SuppressMessage("Microsoft.Design", "CA1051:DoNotDeclareVisibleInstanceFields", Justification = "This is to be a flat object with no function as its a protocol object")]
        public string MessageBody;
        public VirtualChannelResponse() { }
    }

    public enum VirtualChannelResponseCode
    {
        /// <summary>
        /// The response code used to indicate that function was a void function, that has no response information
        /// </summary>
        None,
        /// <summary>
        /// The response code used to indicate that their was a error while executing the appv service code(admin functions)
        /// </summary>
        Error,
        Success,
        AppVClientNotInstalled
    }

    /// <summary>
    /// This enum edientifies the task asked to be carried out by the service
    /// </summary>
    public enum VirtualChannelRequestTask
    {
        // Sync the configuration of the VDA with that of the client endpoint
        SyncConfig,
        // AppVService message will be in the MessageBody of this request 
        ForwardToAppVService,
        // Launch
        Launch,
    }
}
