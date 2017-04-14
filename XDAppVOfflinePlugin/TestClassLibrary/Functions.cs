using System;


namespace TestClassLibrary
{
    public class Functions : Interface
    {
        public int AddAsd(string asd, ref string responseText)
        {
            if (responseText == null) throw new ArgumentNullException("responseText");
            var client = new AppVService((s, s1) => { });
            var response = client.SendMessage(new Request() {MessageBody = asd, RequestTask = RequestTask.AddClientPackage});
            responseText = response.MessageBody;
            return (int)response.ResponseCode;
        }
    }
}
