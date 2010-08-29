using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.IO;
using System.Xml;

namespace NetWorker
{
    public partial class FormSingleDevice : Form
    {

        IPEndPoint rEndpoint, lEndpoint;
        public FormSingleDevice()
        {
            InitializeComponent();

            rEndpoint = new IPEndPoint(IPAddress.Any, 50000);
            lEndpoint = new IPEndPoint(IPAddress.Any, 50000);
        }

        public FormSingleDevice(EpmDeviceDescription target)
            : this()
        {
            rEndpoint = new IPEndPoint(target.remoteIPAddr, target.remotePort);
            Send("<epm><unit><ack/></unit></epm>");
        }

        private void FormSingleDevice_FormClosed(object sender, FormClosedEventArgs e)
        {
            if(receiver != null)
                receiver.Abort();
            Application.Exit();
        }


        void Send(string text)
        {
            UdpClient client = new UdpClient();
            client.Client.Bind(lEndpoint);
            byte[] tx = ASCIIEncoding.Default.GetBytes(text+"\0");

            listboxUpdate(rxSource.send, text);
            client.Send(tx, tx.Length, rEndpoint);
            client.Close();
        }

        bool SendWithAck(string text)
        {
            UdpClient client = new UdpClient();
            client.Client.Bind(lEndpoint);
            bool ret = false;
            byte[] tx = ASCIIEncoding.Default.GetBytes(text + "\0");

            listboxUpdate(rxSource.send, text);
            client.Send(tx, tx.Length, rEndpoint);

            client.Client.ReceiveTimeout = 1000;
            try
            {
                Stream s = new MemoryStream(client.Receive(ref rEndpoint));
                XmlDataDocument doc = new XmlDataDocument();

                doc.Load(s);
                listboxUpdate(rxSource.syncReceive, doc.InnerXml);
                if (!doc["epm"]["unit"]["ack"].HasAttribute("error"))
                    ret = true;
            }
            catch{ }
            
            client.Close();
            return ret;
        }
        private XmlElement SendWithAnswer(string text)
        {
            UdpClient client = new UdpClient();
            client.Client.Bind(lEndpoint);
            XmlElement ret = null;
            byte[] tx = ASCIIEncoding.Default.GetBytes(text + "\0");

            listboxUpdate(rxSource.send, text);
            client.Send(tx, tx.Length, rEndpoint);

            client.Client.ReceiveTimeout = 20000;
            try
            {
                Stream s = new MemoryStream(client.Receive(ref rEndpoint));
                XmlDataDocument doc = new XmlDataDocument();

                doc.Load(s);
                //if (doc["epm"]["unit"]["read"].Attributes["data"].Value == "string")
                listboxUpdate(rxSource.syncReceive, doc.InnerXml);
                ret = doc["epm"];
            }
            catch { }

            client.Close();
            return ret;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            XmlDocument doc = new XmlDocument();
            doc.Load(new MemoryStream(ASCIIEncoding.Default.GetBytes("<epm><unit name=\"SerCom\"><write data=\"string\">" + textBoxSend.Text + "</write></unit></epm>")));

            if (checkBox1.Checked)
            {
                doc["epm"].SetAttribute("ack", "yes");
                if (SendWithAck(doc.InnerXml))
                    pictureBox2.Image = imageListStatus.Images["ok"];
                else
                {
                    pictureBox2.Image = imageListStatus.Images["fail"];
                    return;
                }
            }
            else
            {
                Send(doc.InnerXml);
                pictureBox2.Image = imageListStatus.Images["none"];
            }
            //listboxUpdate(rxSource.send, DateTime.Now, doc["epm"]);
            
        }

        Thread receiver = null;
        bool runReceiver = true;
        private void button2_Click(object sender, EventArgs e)
        {
            receiver = new Thread(new ParameterizedThreadStart(rx));
            receiver.Start(int.Parse(textBoxPort.Text));
            if(SendWithAck("<epm uid=\"123\" ack=\"yes\" reply=\"onchange\" dest=\"192.168.0.1:"+textBoxPort.Text+"\"><unit name=\"SerCom\"><read stx=\"" + (int)ASCIIEncoding.Default.GetBytes(textBoxSTX.Text)[0] + "\" etx=\"" + (int)ASCIIEncoding.Default.GetBytes(textBoxETX.Text)[0] + "\"/></unit></epm>"))
                pictureBox1.Image = imageListStatus.Images["ok"];
            else
                pictureBox1.Image = imageListStatus.Images["fail"];
        }

        void rx(object port)
        {
            UdpClient client = new UdpClient();
            IPEndPoint r = new IPEndPoint(IPAddress.Any, 50000);
            client.Client.Bind(new IPEndPoint(IPAddress.Any, (int)port));
            client.Client.ReceiveTimeout = 1000;
            while (runReceiver)
            {
                try
                {
                    Stream s = new MemoryStream(client.Receive(ref r));
                    XmlDataDocument doc = new XmlDataDocument();

                    doc.Load(s);
                    listboxUpdate(rxSource.asyncReceive, DateTime.Now, doc["epm"]);
                }
                catch { }
            }
            runReceiver = true;
            client.Close();
            Thread.CurrentThread.Abort();
        }

        delegate void listboxUpdater(rxSource source, DateTime now, XmlElement data);
        void listboxUpdate(rxSource source, DateTime now, XmlElement data)
        {
            if(dataGridView1.InvokeRequired)
            {
                dataGridView1.Invoke(new listboxUpdater(listboxUpdate), new object[] { source, now, data });
            }
            else
            {
                Image src = null;
                switch (source)
                {
                    case rxSource.asyncReceive :
                        src = imageListSource.Images["event"];
                        break;
                    case rxSource.send :
                        src = imageListSource.Images["local"];
                        break;
                    case rxSource.syncReceive:
                        src = imageListSource.Images["remote"];
                        if (data.ChildNodes[0].ChildNodes[0].Attributes["error"] != null)
                            src = imageListSource.Images["remote_error"];
                        break;
                }
                dataGridView1.Rows.Add(new object[] { src, now.ToLongTimeString(), data.InnerText, data.OuterXml });
            }
        }
        private void listboxUpdate(rxSource rxSource, string text)
        {
            XmlDocument doc = new XmlDocument();
            doc.Load(new MemoryStream(ASCIIEncoding.Default.GetBytes(text)));
            listboxUpdate(rxSource, DateTime.Now, doc["epm"]);
        }


        private void button1_Click_1(object sender, EventArgs e)
        {
            XmlElement ans = SendWithAnswer("<epm timeout=\""+trackBar1.Value+"\"><unit name=\"SerCom\"><read stx=\"" + (int)ASCIIEncoding.Default.GetBytes(textBoxSTXSync.Text)[0] + "\" etx=\"" + (int)ASCIIEncoding.Default.GetBytes(textBoxETXSync.Text)[0] + "\"/></unit></epm>");

            if (ans != null && ans.ChildNodes[0].ChildNodes[0].Attributes.Count == 0)
            {
                pictureBox3.Image = imageListStatus.Images["ok"];
                listboxUpdate(rxSource.syncReceive, DateTime.Now, ans);
            }
            else
            {
                pictureBox3.Image = imageListStatus.Images["fail"];
            }
        }



        private void button3_Click(object sender, EventArgs e)
        {
            if (SendWithAck("<epm uid=\"123\" ack=\"yes\" reply=\"never\"><unit name=\"SerCom\"><read/></unit></epm>"))
                pictureBox4.Image = imageListStatus.Images["ok"];
            else
                pictureBox4.Image = imageListStatus.Images["fail"];

            runReceiver = false;
        }

        private void trackBar1_Scroll(object sender, EventArgs e)
        {
            label6.Text = string.Format("{0}: {1}ms", (string)label6.Tag, trackBar1.Value);
        }
    }
    enum rxSource
    {
        asyncReceive,
        syncReceive,
        send
    }
}
