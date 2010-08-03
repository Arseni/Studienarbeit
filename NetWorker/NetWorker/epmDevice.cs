using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.Threading;
using System.Net.Sockets;
using System.IO;
using System.Xml;
using System.Windows.Forms;

namespace NetWorker
{
    internal class EpmDevice
    {
        private static List<EpmDeviceDescription> deviceList = new List<EpmDeviceDescription>();
        public static event EventHandler<EpmDeviceFoundEventArgs> OnDeviceFound;
        static Thread searcher;
        public static void SearchEpmDeviceStart()
        {
            if (searcher != null && searcher.IsAlive)
                searcher.Abort();
            searcher = new Thread(new ThreadStart(search));
            searcher.Name = "Searcher";
            if (OnDeviceFound == null)
                return;

            searcher.Start();
        }
        public static void SearchEpmDeviceStop()
        {
            searcher.Abort();
        }
        public static void search()
        {
            IPEndPoint rEndpoint = new IPEndPoint(IPAddress.Any/*.Parse("192.168.0.13")*/, 50000);
            IPEndPoint lEndpoint = new IPEndPoint(IPAddress.Any, 50001);

            using (UdpClient client = new UdpClient())
            {
                client.Client.Bind(lEndpoint);
                client.Client.ReceiveTimeout = 1000;

                while (true)
                {
                    try
                    {
                        Stream s = new MemoryStream(client.Receive(ref rEndpoint));
                        XmlDataDocument doc = new XmlDataDocument();
                    
                        doc.Load(s);
                        EpmDeviceDescription newDevice = new EpmDeviceDescription();
                        string ss = doc.InnerXml;
                        string[] rHome = doc["epm"].Attributes["home"].Value.Split(":".ToCharArray());
                        newDevice.remoteIPAddr = IPAddress.Parse(rHome[0]);
                        newDevice.remotePort = int.Parse(rHome[1]);
                        List<string> units = new List<string>();

                        foreach (XmlNode i in doc["epm"].ChildNodes)
                        {
                            units.Add(i.Attributes["name"].Value);
                        }
                        newDevice.units = (string[])units.ToArray();

                        lock (deviceList)
                        {
                            foreach (EpmDeviceDescription i in deviceList)
                            {
                                if (newDevice.remoteIPAddr.Address == i.remoteIPAddr.Address &&
                                   newDevice.remotePort == i.remotePort)
                                    throw new Exception();
                            }
                            deviceList.Add(newDevice);
                            OnDeviceFound(null, new EpmDeviceFoundEventArgs(newDevice));
                        }
                    }
                    catch { }
                }
            }
        }
    }
    public class EpmDeviceDescription
    {
        public IPAddress remoteIPAddr;
        public int remotePort;
        public string[] units;

        public override string ToString()
        {
            return remoteIPAddr.ToString();
        }
    }
    public class EpmDeviceFoundEventArgs : EventArgs
    {
        public EpmDeviceDescription device { get; private set; }
        public EpmDeviceFoundEventArgs(EpmDeviceDescription p)
        {
            device = p;
        }
    }
}
