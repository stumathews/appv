using System;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;

namespace TestClassLibrary
{
    public class TestClassInterfaceFunctions : TestClassInterface
    {
        public string ProcessVirtualChannelRequest(string payload)
        {          
          // Before we ask the AppV Service to do anything, lets see if we've got the appv client installed first.
          Version m_AppVSP1ClientVersion = new Version(5, 0, 1104, 0);
          if (!(AppVClientInfo.IsAppVClientPresent(new Version(5, 0, 0, 0), AppVVersionCheckCriteria.OnlyMajor) == true) && AppVClientInfo.IsAppVClientPresent(m_AppVSP1ClientVersion, AppVVersionCheckCriteria.GreaterThan))
          {
              return ServiceBrokerProtocolHelper.Serialize(new VirtualChannelResponse { ResponseCode = VirtualChannelResponseCode.AppVClientNotInstalled } );
          }

          // Now lets deal with the virtual channel request
          
          
          string ret = string.Empty;          
          try
          {    
            var response = new VirtualChannelResponse();        
            var request = ServiceBrokerProtocolHelper.Deserialize<VirtualChannelRequest>(payload);
            string keyName = @"HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Citrix\AppLibrary";
            switch(request.RequestTask)
            {
                case VirtualChannelRequestTask.SyncConfig:
                    // Get config to sync
                    var config = ServiceBrokerProtocolHelper.Deserialize<Config>(request.MessageBody);
                    // Set/Overwrite (key is created if not exists)
                    Registry.SetValue(keyName, "ApplicationStartDetails", config.Asds, RegistryValueKind.MultiString);
                    Registry.SetValue(keyName, "IsolationGroupStartDetails", config.Isds, RegistryValueKind.MultiString);
            
                    // What we should do now is actually add the isolation groups if we need to or add packages if we need to.
                    // ** This could be slow even if its already added/created so might need to optimise this process **
                    // Add the packages first
                    foreach(var asd in config.Asds)
                    {
                        var addAsdRequest = new Request { RequestTask = RequestTask.AddClientPackage, MessageBody = asd };
                        var addAsdResponse = ForwardRequestToAppVService(addAsdRequest);
                    }
                    
                    // Add the Isolation groups afterwards
                    foreach(var isd in config.Isds)
                    {
                        var addIsdRequest = new Request { RequestTask = RequestTask.UpdateIsolationGroups, MessageBody = isd };
                        var addIsdResponse = ForwardRequestToAppVService(addIsdRequest);
                    }
                    response.ResponseCode = VirtualChannelResponseCode.Success;                                    
                    break;
                case VirtualChannelRequestTask.ForwardToAppVService:
                    // Extract the request to send to CtxAppVService
                    var ctxAppVServiceRequest = ServiceBrokerProtocolHelper.Deserialize<Request>(request.MessageBody);
                    // Send the request off to the service
                    var ctxAppVResponse = ForwardRequestToAppVService(ctxAppVServiceRequest);
                    response.ResponseCode = (ctxAppVResponse.ResponseCode != ResponseCode.Error) ? VirtualChannelResponseCode.Success : VirtualChannelResponseCode.Error;                 
                    break;
                case VirtualChannelRequestTask.Launch:
                    // We'll get the ASD from the MessageBody or a 
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
