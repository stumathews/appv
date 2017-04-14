using System;


namespace TestClassLibrary
{
    public class TestClassInterfaceFunctions : TestClassInterface
    {
        public int AddAsd(string asd, ref string responseText)
        {
            if (responseText == null) throw new ArgumentNullException("responseText");
            var client = new AppVService((s, s1) => { });
            var response = client.SendMessage(new Request() {MessageBody = asd, RequestTask = RequestTask.AddClientPackage});
            responseText = response.MessageBody;
            return (int)response.ResponseCode;
        }

    public string echo( string msg )
    {
      return msg;
    }

    public int return5()
    {
      return 5;
    }

    public string returnHello()
    {
      return "Hello";
    }
  }
}
