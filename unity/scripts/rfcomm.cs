using System.Collections;
using System.Collections.Generic;
using System.IO.Ports;
using UnityEngine;

public class rfcomm : MonoBehaviour {
	static SerialPort sp = new SerialPort();
	static int port_l    = 0;
	static int port_u    = 10;
	
	public string m_SerialPortName = "";
	public string message_write;
	public bool   send    = false;
	public bool   disable = false;

	void Start()
	{
		sp.BaudRate     = 9600;
		sp.Parity       = Parity.None;
		sp.DataBits     = 8;
		sp.StopBits     = StopBits.One;
		sp.WriteTimeout = 5000;
	}

	void Update()
	{
		if (disable)
			return;

		if (send)
		{
			Send(message_write);
			send = false;
		}
	}

	public void Send(string message)
	{
		if (disable)
			return;

		OpenConnection();
		sp.WriteLine(message);
	}

	public void OpenConnection()
	{
		if (disable)
			return;

		if (sp == null)
		{
			Debug.Log("Port == null");
			return;
		}

		if (sp.IsOpen)
		{
			Debug.Log("Port is Open");
			return;
		}

		if (m_SerialPortName != "")
		{
			if (tryPort(m_SerialPortName) == 0)
				return;
		}


		var m_PortNames = SerialPort.GetPortNames();
		foreach(string port in m_PortNames)
		{
			if (tryPort(port) == 0)
				return;
		}
	}
	
	private int tryPort(string portName)
	{		
		sp.PortName = portName;
		try {
			sp.WriteTimeout = 5000;
			sp.Open();
			sp.WriteLine("0 0");
		} catch {
			sp.Close();
			return 1;
		}
		return 0;
	}

	void OnApplicationQuit() {
		sp.Close();
	}

}
