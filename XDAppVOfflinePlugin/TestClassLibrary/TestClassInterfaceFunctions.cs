using System;


namespace TestClassLibrary
{
    public class TestClassInterfaceFunctions : TestClassInterface
    {
        public int AddAsd(string asd)
        {
            
            var client = new AppVService((s, s1) => { });
            var response = client.SendMessage(new Request() {MessageBody = "test", RequestTask = RequestTask.AddClientPackage});
            //SresponseText = response.MessageBody;
            return (int)6;
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
