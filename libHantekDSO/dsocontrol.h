////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file dsocontrol.h
/// \brief Declares the abstract Control class.
//
//  Copyright (C) 2010  Oliver Haag
//  oliver.haag@gmail.com
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////


#ifndef DSOCONTROL_H
#define DSOCONTROL_H


#include <vector>

#include <QString>
#include <QThread>
class QMutex;

namespace Dso {
	//////////////////////////////////////////////////////////////////////////////
	/// \enum ErrorCode                                           hantek/control.h
	/// \brief The return codes for device control methods.
	enum ErrorCode {
		ERROR_NONE = 0, ///< Successful operation
		ERROR_CONNECTION = -1, ///< Device not connected or communication error
		ERROR_UNSUPPORTED = -2, ///< Not supported by this device
		ERROR_PARAMETER = -3 ///< Parameter out of range
	};

	//////////////////////////////////////////////////////////////////////////////
	/// \enum ChannelMode                                                    dso.h
	/// \brief The channel display modes.
	enum ChannelMode {
		CHANNELMODE_VOLTAGE,                ///< Standard voltage view
		CHANNELMODE_SPECTRUM,               ///< Spectrum view
		CHANNELMODE_COUNT                   ///< The total number of modes
	};

	//////////////////////////////////////////////////////////////////////////////
	/// \enum GraphFormat                                                    dso.h
	/// \brief The possible viewing formats for the graphs on the scope.
	enum GraphFormat {
		GRAPHFORMAT_TY,                     ///< The standard mode
		GRAPHFORMAT_XY,                     ///< CH1 on X-axis, CH2 on Y-axis
		GRAPHFORMAT_COUNT                   ///< The total number of formats
	};

	//////////////////////////////////////////////////////////////////////////////
	/// \enum Coupling                                                       dso.h
	/// \brief The coupling modes for the channels.
	enum Coupling {
		COUPLING_AC,                        ///< Offset filtered out by condensator
		COUPLING_DC,                        ///< No filtering
		COUPLING_GND,                       ///< Channel is grounded
		COUPLING_COUNT                      ///< The total number of coupling modes
	};

	//////////////////////////////////////////////////////////////////////////////
	/// \enum MathMode                                                       dso.h
	/// \brief The different math modes for the math-channel.
	enum MathMode {
		MATHMODE_1ADD2,                     ///< Add the values of the channels
		MATHMODE_1SUB2,                     ///< Subtract CH2 from CH1
		MATHMODE_2SUB1,                     ///< Subtract CH1 from CH2
		MATHMODE_COUNT                      ///< The total number of math modes
	};

	//////////////////////////////////////////////////////////////////////////////
	/// \enum TriggerMode                                                    dso.h
	/// \brief The different triggering modes.
	enum TriggerMode {
		TRIGGERMODE_AUTO,                   ///< Automatic without trigger event
		TRIGGERMODE_NORMAL,                 ///< Normal mode
		TRIGGERMODE_SINGLE,                 ///< Stop after the first trigger event
		TRIGGERMODE_COUNT                   ///< The total number of modes
	};

	//////////////////////////////////////////////////////////////////////////////
	/// \enum Slope                                                          dso.h
	/// \brief The slope that causes a trigger.
	enum Slope {
		SLOPE_POSITIVE,                     ///< From lower to higher voltage
		SLOPE_NEGATIVE,                     ///< From higher to lower voltage
		SLOPE_COUNT                         ///< Total number of trigger slopes
	};

	//////////////////////////////////////////////////////////////////////////////
	/// \enum WindowFunction                                                 dso.h
	/// \brief The supported window functions.
	/// These are needed for spectrum analysis and are applied to the sample values
	/// before calculating the DFT.
	enum WindowFunction {
		WINDOW_RECTANGULAR,                 ///< Rectangular window (aka Dirichlet)
		WINDOW_HAMMING,                     ///< Hamming window
		WINDOW_HANN,                        ///< Hann window
		WINDOW_COSINE,                      ///< Cosine window (aka Sine)
		WINDOW_LANCZOS,                     ///< Lanczos window (aka Sinc)
		WINDOW_BARTLETT,                    ///< Bartlett window (Endpoints == 0)
		WINDOW_TRIANGULAR,                  ///< Triangular window (Endpoints != 0)
		WINDOW_GAUSS,                       ///< Gauss window (simga = 0.4)
		WINDOW_BARTLETTHANN,                ///< Bartlett-Hann window
		WINDOW_BLACKMAN,                    ///< Blackman window (alpha = 0.16)
		//WINDOW_KAISER,                      ///< Kaiser window (alpha = 3.0)
		WINDOW_NUTTALL,                     ///< Nuttall window, cont. first deriv.
		WINDOW_BLACKMANHARRIS,              ///< Blackman-Harris window
		WINDOW_BLACKMANNUTTALL,             ///< Blackman-Nuttall window
		WINDOW_FLATTOP,                     ///< Flat top window
		WINDOW_COUNT                        ///< Total number of window functions
	};

	////////////////////////////////////////////////////////////////////////////////
	/// \enum InterpolationMode                                                dso.h
	/// \brief The different interpolation modes for the graphs.
	enum InterpolationMode {
		INTERPOLATION_OFF = 0,              ///< Just dots for each sample
		INTERPOLATION_LINEAR,               ///< Sample dots connected by lines
		INTERPOLATION_SINC,                 ///< Smooth graph through the dots
		INTERPOLATION_COUNT                 ///< Total number of interpolation modes
	};
}

/// \class DsoControl
/// \brief An abstraction layer that enables protocol-independent dso usage.
class DsoControl : public QThread {
	Q_OBJECT
	
	public:
		DsoControl(QObject *parent = 0);
		
		virtual unsigned int getChannelCount() = 0; ///< Get the number of channels for this oscilloscope
		virtual std::vector<unsigned> *getAvailableRecordLengths() = 0; ///< Get available record lengths, empty list for continuous
		virtual double getMinSamplerate() = 0; ///< The minimum samplerate supported
		virtual double getMaxSamplerate() = 0; ///< The maximum samplerate supported
		
		const std::vector<QString> *getSpecialTriggerSources();
	
	protected:
		bool sampling; ///< true, if the oscilloscope is taking samples
		
		std::vector<QString> specialTriggerSources; ///< Names of the special trigger sources
		
	signals:
		void deviceConnected(); ///< The oscilloscope device has been disconnected
		void deviceDisconnected(); ///< The oscilloscope device has been connected
		void samplingStarted(); ///< The oscilloscope started sampling/waiting for trigger
		void samplingStopped(); ///< The oscilloscope stopped sampling/waiting for trigger
		void statusMessage(const QString &message, int timeout); ///< Status message about the oscilloscope
		void samplesAvailable(const std::vector<std::vector<double> > *data, double samplerate, bool append, QMutex *mutex); ///< New sample data is available
		
		void availableRecordLengthsChanged(const std::vector<unsigned> &recordLengths); ///< The available record lengths, empty list for continuous
		void samplerateLimitsChanged(double minimum, double maximum); ///< The minimum or maximum samplerate has changed
		void recordLengthChanged(unsigned long duration); ///< The record length has changed
		void recordTimeChanged(double duration); ///< The record time duration has changed
		void samplerateChanged(double samplerate); ///< The samplerate has changed
	
	public slots:
		virtual void connectDevice();
		virtual void disconnectDevice();
		
		virtual void startSampling();
		virtual void stopSampling();
		
		virtual unsigned int setRecordLength(unsigned int size) = 0; ///< Set record length id, minimum for continuous
		virtual double setSamplerate(double samplerate) = 0; ///< Set the samplerate that should be met
		virtual double setRecordTime(double duration) = 0; ///< Set the record time duration that should be met
		
		virtual int setTriggerMode(Dso::TriggerMode mode) = 0; ///< Set the trigger mode
		virtual int setTriggerSource(bool special, unsigned int id) = 0; ///< Set the trigger source
		virtual double setTriggerLevel(unsigned int channel, double level) = 0; ///< Set the trigger level for a channel
		virtual int setTriggerSlope(Dso::Slope slope) = 0; ///< Set the slope that causes triggering
		virtual double setPretriggerPosition(double position) = 0; ///< Set the pretrigger position (0.0 = left, 1.0 = right side)
		virtual int forceTrigger() = 0;
		
		virtual int setChannelUsed(unsigned int channel, bool used) = 0; ///< Enable/disable a channel
		virtual int setCoupling(unsigned int channel, Dso::Coupling coupling) = 0; ///< Set the coupling for a channel
		virtual double setGain(unsigned int channel, double gain) = 0; ///< Set the gain for a channel
		virtual double setOffset(unsigned int channel, double offset) = 0; ///< Set the graph offset of a channel
		
#ifdef DEBUG
		virtual int stringCommand(QString command) = 0; ///< Sends commands directly, for debugging
#endif
};


#endif
