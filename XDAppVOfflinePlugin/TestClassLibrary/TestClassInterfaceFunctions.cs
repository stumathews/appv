using System;
using System.Diagnostics;
using System.IO;
using Citrix.VirtApp.Vda.Common;
using Microsoft.Win32;

namespace TestClassLibrary
{
    public class TestClassInterfaceFunctions : TestClassInterface
    {
        public string ProcessVirtualChannelRequest(string payload)
        {          
          // Before we ask the AppV Service to do anything, lets see if we've got the appv client installed first.
          Version m_AppVSP1ClientVersion = new Version(5, 0, 1104, 0);
          if (!(AppVClientInfo.IsAppVClientPresent(new Version(5, 0, 0, 0), AppVVersionCheckCriteria.OnlyMajor) == true) &&
                AppVClientInfo.IsAppVClientPresent(m_AppVSP1ClientVersion, AppVVersionCheckCriteria.GreaterThan))
          {
              return ServiceBrokerProtocolHelper.Serialize(new VirtualChannelResponse { ResponseCode = VirtualChannelResponseCode.AppVClientNotInstalled } );
          }

          // Now lets deal with the incomming virtual channel request from the launcher on the VDA
                   
          try
          {    
            var response = new VirtualChannelResponse();        
            var request = ServiceBrokerProtocolHelper.Deserialize<VirtualChannelRequest>(payload);

            switch(request.RequestTask)
            {                
                case VirtualChannelRequestTask.ForwardToAppVService:
                    // Extract the request to send to CtxAppVService, the message body contains wrapped in it, the request for ctxappvservice
                    var ctxAppVServiceRequest = ServiceBrokerProtocolHelper.Deserialize<Request>(request.MessageBody);
                    // Send the request off to the service
                    var ctxAppVResponse = ForwardRequestToAppVService(ctxAppVServiceRequest);
                    response.ResponseCode = (ctxAppVResponse.ResponseCode != ResponseCode.Error) ? VirtualChannelResponseCode.Success : VirtualChannelResponseCode.Error;                 
                    break;
                case VirtualChannelRequestTask.Launch:
                    // the command line is the messagebody
                    var argument = request.MessageBody;
                    var ctxAppXLauncher = new Process { StartInfo = { FileName = "ctxappvlauncher.exe", Arguments = argument } };
                    ctxAppXLauncher.Start();
                    break;
            }            
            return ServiceBrokerProtocolHelper.Serialize<VirtualChannelResponse>(response);
          }
          catch(Exception e)
          {
              const int max_length = 200;
              string message = "Er:"+e.Message + e.StackTrace;        
              return message.Length <= max_length ? message : message.Substring(0,max_length);
          }
        }

          private static Response ForwardRequestToAppVService( Request request )
          {
            // This client requires that the CtxAppVService disable name pipe security to work.
            var client = new AppVService( ( methodName, message ) => { } );

            // The actual request is wrapped within the messagebody of the VirtualChannel request
            var ctxAppVServiceResponse = client.SendMessage(request);
            return ctxAppVServiceResponse;
          }
  }
}
