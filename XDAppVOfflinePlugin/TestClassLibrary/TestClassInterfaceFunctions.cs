﻿using System;
using System.IO;

namespace TestClassLibrary
{
    public class TestClassInterfaceFunctions : TestClassInterface
    {
        public string SendToAppVService(string payload)
        {
          string ret;
          try
          {
            var request = ServiceBrokerProtocolHelper.Deserialize<Request>(payload);
            // This client requires that the CtxAppVService disable name pipe security to work.
            var client = new AppVService( ( methodName, message ) => {
              //  string tempPath = Path.GetTempPath();
              //  var line = string.Format("{0} : {1}",methodName,message);
              //File.AppendAllText("TestClassLibrary", line );
            });
            var response = client.SendMessage(request);
            ret = ServiceBrokerProtocolHelper.Serialize(response);
            return ret;
          }
          catch(Exception e)
          {
              const int max_length = 200;
              string message = "Er:"+e.Message + e.StackTrace;        
              return message.Length <= max_length ? message : message.Substring(0,max_length);
          }
        }
        /* Legacy Test functions - To be removed in the future. */

        public int AddAsd(string asd)
            {
                try
                {
                  var client = new AppVService((s, s1) => { });
                  var response = client.SendMessage(new Request() {MessageBody = "test", RequestTask = RequestTask.AddClientPackage});
                  return (int)response.MessageBody.Length;
                }
                catch(Exception e)
                {
                  return -1;
                }      
            }

        public string echo( string msg )
        {
          string ret = string.Empty;
          try
          {
             var client = new AppVService((s, s1) => {} );
             var response = client.SendMessage(new Request() {MessageBody = msg, RequestTask = RequestTask.AddClientPackage});
             ret = string.Format("{0}{1}", response.ResponseCode, response.MessageBody);
          }
          catch(Exception e)
          {
            const int max_length = 200;
            string message = e.Message + e.StackTrace;
        
            return message.Length <= max_length? message : message.Substring(0,max_length);
          }
          return string.IsNullOrEmpty(ret) ? "Hello" : ret;    
        }

        public int return5()
        {
          return 5;
        }

        public string returnHello()
        {
          string ret = string.Empty;
          try
          {
             var client = new AppVService((s, s1) => {} );
             var response = client.SendMessage(new Request() {MessageBody = @"462dfa90-9e09-429e-a6f3-00cb3e9bf90b;\\londondc.london.local\appvshare\EPM 3.appv;4148dc52-d7d5-47c3-a570-396aa63fa9fe;d2022911-4251-4c86-b8cd-e2bb092443fd;EPM Opsætningsmodul", RequestTask = RequestTask.AddClientPackage});
             ret = string.Format("{0}{1}", response.ResponseCode, response.MessageBody);
          }
          catch(Exception e)
          {
            const int max_length = 200;
            string message = e.Message + e.StackTrace;
        
            return message.Length <= max_length? message : message.Substring(0,max_length);
          }
          return string.IsNullOrEmpty(ret) ? "Hello" : ret;
        }

        public string AddClientPackage(string payload)
        {
          string ret = string.Empty;
          RequestTask task = RequestTask.AddClientPackage;

          return string.IsNullOrEmpty(ret) ? "Hello" : ret;    

        }
  }
}
