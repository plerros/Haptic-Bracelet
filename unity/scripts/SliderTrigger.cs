using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using UnityEngine.Events;

namespace UnityEngine.UI {
	public class SliderTrigger : MonoBehaviour
	{
		private Slider mySlider;
		private System.DateTime lastMoveTime = DateTime.Now;
		private bool m_Selected = false;

		public UnityEvent myEvent;
		public int targetValue;

		public UnityEvent m_SelectedEvent;
	
		// Start is called before the first frame update
		void Start()
		{
			mySlider = GetComponent<Slider>();
		}

		// Update is called once per frame
		void Update()
		{
			if (myEvent == null)
				return;

			if (mySlider.value != targetValue)
				return;

			if (m_Selected == false)
				return;

			// allow the user to overshoot
			if ((DateTime.Now - lastMoveTime).TotalSeconds < 0.1)
				return;

			myEvent.Invoke();
			m_Selected = false;
		}

		public void SliderMoved()
		{
			lastMoveTime = DateTime.Now;
			if (m_Selected == false)
				m_SelectedEvent.Invoke();

			m_Selected = true;
		}

	}
}