#include "xmlconfighandler.h"

#pragma warning(push,3)
#include <QCoreApplication>
#include <QFile>
#include <QMutexLocker>
#include <QTextStream>
#pragma warning(pop)

#define DEFAULT_DATABASE_DIR				"C:/ResearchPlatform/database"
#define DEFAULT_INPUT_DIR						"C:/ResearchPlatform/dbupdater/update"
#define DEFAULT_RESULT_DIR					"C:/ResearchPlatform/tools/results"
#define DEFAULT_STRATEGY_DIR				"C:/Users/Jansen/Documents/Documents/strategy"
#define DEFAULT_SEARCHPATTERN_DIR   "C:/Users/Jansen/Documents/Documents/strategy/searchpattern"
#define DEFAULT_PROCESS_DIR					""

#define TOKEN_UPDATE_PARENT_INDEX         "updateparentindex"
#define TOKEN_UPDATE_SUPPORT_RESISTANCE   "updatesupportresistance"
#define TOKEN_UPDATE_HISTOGRAM            "updatehistogram"
#define TOKEN_DEVELOPER_MODE              "developermode"
#define TOKEN_INITIAL_CAPITAL   "initial_capital"
#define TOKEN_REDUCE_NUMBER     "reducenumber"
#define TOKEN_DATABASE_DIR      "databasedir"
#define TOKEN_INPUT_DIR         "inputdir"
#define TOKEN_RESULT_DIR        "resultdir"
#define TOKEN_PROCESS_DIR       "processdir"
#define TOKEN_TS_STRATEGY_DIR   "strategydir"
#define TOKEN_SEARCHPATTERN_DIR "searchpatterndir"
#define TOKEN_IPCNAME           "ipcname"
#define TOKEN_TIMER_INTERVAL    "timerinterval"
#define TOKEN_LAST_UPDATE       "lastupdate"
#define TOKEN_SYMBOL            "symbol"
#define TOKEN_INTERVAL          "interval"
#define TOKEN_THRESHOLD         "thresholds"
#define TOKEN_MACD              "macd"
#define TOKEN_RSI               "rsi"
#define TOKEN_SLOWK             "slowk"
#define TOKEN_SLOWD             "slowd"
#define TOKEN_LOOKAHEAD         "lookahead"
#define TOKEN_CGF               "cgf"
#define TOKEN_CLF               "clf"
#define TOKEN_CGS               "cgs"
#define TOKEN_CLS               "cls"
#define TOKEN_DISTFSCROSS       "distfscross"
#define TOKEN_FGS               "fgs"
#define TOKEN_FLS               "fls"
#define TOKEN_TWS               "tws"
#define TOKEN_TWS_IP            "tws_ip"
#define TOKEN_TWS_PORT          "tws_port"
#define TOKEN_TWS_ID            "tws_id"
#define TOKEN_TWS_MAPPING       "mapping"
#define TOKEN_TWS_SYMBOL        "symbol"
#define TOKEN_TWS_LOCAL_SYMBOL  "local_symbol"
#define TOKEN_TWS_TYPE          "type"
#define TOKEN_TWS_EXCHANGE      "exchange"
#define TOKEN_RANK_LASTDATE     "rank_lastdate"

#define MAX_SUPPORT_RESISTANCE_THRESHOLD  5
#define MAX_CLOSE_THRESHOLD               5

#define _XML_VALUE            xml.attributes().value("value").toString()
#define _XML_VALUE_INT        xml.attributes().value("value").toInt()
#define _XML_VALUE_DOUBLE     xml.attributes().value("value").toDouble()
#define _XML_VALUE_STDSTRING  xml.attributes().value("value").toString().toStdString()
#define _XML_TWS_IP           xml.attributes().value("ip").toString()
#define _XML_TWS_PORT         xml.attributes().value("port").toString()
#define _XML_TWS_ID           xml.attributes().value("id").toString()

std::shared_ptr<XmlConfigHandler> XmlConfigHandler::_instance = nullptr;
std::once_flag XmlConfigHandler::_only_one;

void XmlConfigHandler::initialize_variable()
{
	config_properties.clear();
	config_properties.insert(TOKEN_DATABASE_DIR, DEFAULT_DATABASE_DIR);
	config_properties.insert(TOKEN_INPUT_DIR, DEFAULT_INPUT_DIR);
	config_properties.insert(TOKEN_RESULT_DIR, DEFAULT_RESULT_DIR);
	config_properties.insert(TOKEN_PROCESS_DIR, DEFAULT_PROCESS_DIR);
	config_properties.insert(TOKEN_TS_STRATEGY_DIR, DEFAULT_STRATEGY_DIR);
	config_properties.insert(TOKEN_SEARCHPATTERN_DIR, DEFAULT_SEARCHPATTERN_DIR);
	config_properties.insert(TOKEN_IPCNAME, "dbupdater");
	config_properties.insert(TOKEN_INITIAL_CAPITAL, "1000000");
	config_properties.insert(TOKEN_REDUCE_NUMBER, "1");
	config_properties.insert(TOKEN_TIMER_INTERVAL, "5");
	config_properties.insert(TOKEN_TWS_ID, "");
	config_properties.insert(TOKEN_TWS_PORT, "");
	config_properties.insert(TOKEN_TWS_IP, "");
	config_properties.insert(TOKEN_RANK_LASTDATE, "2015-12-31");

	enable_developer_mode = false;
	enable_parent_indexing = true;
	enable_update_support_resistance = true;
	enable_update_histogram = true;

	list_symbol.clear();
	list_sr_threshold.clear();
	interval_weight.clear();
	interval_threshold.clear();
	t_macd.clear();
	t_rsi.clear();
	t_slowk.clear();
	t_slowd.clear();
	t_cgf.clear();
	t_clf.clear();
	t_cgs.clear();
	t_cls.clear();
	t_lookahead.clear();
	t_distfscross.clear();
	t_fgs.clear();
	t_fls.clear();
	tws_mapping.clear();
}

inline void appendXmlElement (QXmlStreamWriter *stream, const QString &element, const QString &value)
{
	stream->writeStartElement(element);
	stream->writeAttribute("value", value);
	stream->writeEndElement();
}

inline void begin_TWS_XMLElement (QXmlStreamWriter *stream, const QString &ip, const QString &port, const QString &id)
{
	stream->writeStartElement(TOKEN_TWS);
	stream->writeAttribute("ip", ip);
	stream->writeAttribute("port", port);
	stream->writeAttribute("id", id);
}

inline void end_TWS_XMLElement (QXmlStreamWriter *stream)
{
	stream->writeEndElement();
}

inline void append_TWS_mapping (QXmlStreamWriter *stream, const QString &symbol, const QString &local_symbol, const QString &type, const QString &exchange)
{
	stream->writeStartElement(TOKEN_TWS_MAPPING);
	stream->writeAttribute("symbol", symbol);
	stream->writeAttribute("local_symbol", local_symbol);
	stream->writeAttribute("type", type);
	stream->writeAttribute("exchange", exchange);
	stream->writeEndElement();
}

inline void appendXmlElement_Interval (QXmlStreamWriter *stream, const QString &interval, const QString &weight, const QString &threshold)
{
	stream->writeStartElement("item");
	stream->writeAttribute("name", interval);
	stream->writeAttribute("weight", weight);
	stream->writeAttribute("threshold", threshold);
	stream->writeEndElement();
}

inline void appendXmlElement_Threshold_1 (QXmlStreamWriter *stream, const QString &id, const QString &_operator1, const QString &value1, const QString &_operator2, const QString &value2)
{
	stream->writeStartElement("item");
	stream->writeAttribute("id", id);
	stream->writeAttribute("operator1", _operator1);
	stream->writeAttribute("value1", value1);
	stream->writeAttribute("operator2", _operator2);
	stream->writeAttribute("value2", value2);
	stream->writeEndElement();
}

inline void appendXmlElement_Threshold_2 (QXmlStreamWriter *stream, const QString &id, const QString &value)
{
	stream->writeStartElement("item");
	stream->writeAttribute("id", id);
	stream->writeAttribute("value", value);
	stream->writeEndElement();
}

/*void appendXmlElement_SupportResistance (QXmlStreamWriter *stream, const QString &symbol, const QString &index,
																				const QString &threshold, const QString &b, const QString &recalculate) {
	stream->writeStartElement("market");
	stream->writeAttribute("symbol", symbol);

	stream->writeAttribute("index", index);
	stream->writeAttribute("threshold", threshold);
	stream->writeAttribute("b", b);
	stream->writeAttribute("recalculate", recalculate);

	stream->writeEndElement();
}*/

inline void appendXmlElement_SupportResistance_V2 (QXmlStreamWriter *stream, const QString &index, const QString &test_point,
																									 const QString &break_threshold, const QString &recalculate)
{
	stream->writeStartElement("param");
	stream->writeAttribute("index", index);
	stream->writeAttribute("test_point", test_point);
	stream->writeAttribute("break_threshold", break_threshold);
	stream->writeAttribute("recalculate", recalculate);
	stream->writeEndElement();
}

QVector<double> lookup_close_threshold (const QString &symbol)
{
	QVector<double> data;

	if (symbol == "@AD")
	{
		data.push_back(0);
		data.push_back(0.001);
		data.push_back(0.002);
	}

	else if (symbol == "@CL")
	{
		data.push_back(0);
		data.push_back(0.1);
		data.push_back(0.2);
		data.push_back(0.4);
	}

	else if (symbol == "@EC")
	{
		data.push_back(0);
		data.push_back(0.001);
		data.push_back(0.002);
	}

	else if (symbol == "@ES")
	{
		data.push_back(0);
		data.push_back(1);
		data.push_back(2);
		data.push_back(4);
	}

	else if (symbol == "@GC")
	{
		data.push_back(0);
		data.push_back(1);
		data.push_back(2);
	}

	else if (symbol == "@JY")
	{
		data.push_back(0);
		data.push_back(0.001);
		data.push_back(0.002);
	}

	else if (symbol == "@NIY")
	{
		data.push_back(0);
		data.push_back(10);
		data.push_back(20);
		data.push_back(40);
	}

	else if (symbol == "@NQ")
	{
		data.push_back(0);
		data.push_back(2);
		data.push_back(4);
		data.push_back(8);
	}

	else if (symbol == "HHI")
	{
		data.push_back(0);
		data.push_back(10);
		data.push_back(20);
		data.push_back(40);
		data.push_back(80);
	}

	else if (symbol == "HSI")
	{
		data.push_back(0);
		data.push_back(20);
		data.push_back(10);
		data.push_back(40);
		data.push_back(80);
	}

	else if (symbol == "IFB")
	{
		data.push_back(0);
		data.push_back(4);
		data.push_back(8);
	}

	else if (symbol == "@SI")
	{
		data.push_back(0);
		data.push_back(0.01);
		data.push_back(0.02);
		data.push_back(0.04);
	}

	return data;
}


//
// XML PARSER
//
void XmlConfigHandler::xml_parse_interval (QXmlStreamReader &xml)
{
	QString interval;
	int thres, weight;
	int counter = 0;

	xml.readNext();

	while (++counter < 200 && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toLower() == TOKEN_INTERVAL))
	{
		if (xml.tokenType() == QXmlStreamReader::StartElement)
		{
			if (xml.name() == "item")
			{
				interval = xml.attributes().value("name").toString();
				weight = xml.attributes().value("weight").toInt();
				thres = xml.attributes().value("threshold").toInt();
				interval_weight.insert(weight, interval);
				interval_threshold.insert(weight, thres);
			}
		}

		xml.readNext();
	}
}

void XmlConfigHandler::xml_parse_close_fs (QXmlStreamReader &xml, const QString &symbol, const QString &token_name)
{
	QVector<t_threshold_2> vec;
	t_threshold_2 data;
	int counter;

	for (int i = 1; i <= MAX_CLOSE_THRESHOLD; ++i)
	{
		// try to avoid infinite loop
		counter = 0;
		while (++counter < 300 && !xml.readNextStartElement()) /** do nothing **/;

		data.t_id = xml.attributes().value("id").toInt();
		data.value = xml.attributes().value("value").toString();
		vec.push_back(data);
	}

	if (token_name == TOKEN_CGF) t_cgf.insert(symbol, vec); else
	if (token_name == TOKEN_CLF) t_clf.insert(symbol, vec); else
	if (token_name == TOKEN_CGS) t_cgs.insert(symbol, vec); else
	if (token_name == TOKEN_CLS) t_cls.insert(symbol, vec); else
	if (token_name == TOKEN_DISTFSCROSS) t_distfscross.insert(symbol, vec); else
	if (token_name == TOKEN_FGS) t_fgs.insert(symbol, vec); else
	if (token_name == TOKEN_FLS) t_fls.insert(symbol, vec);
}

void XmlConfigHandler::xml_parse_threshold (QXmlStreamReader &xml)
{
	QVector<t_sr_threshold> thres;
	QString symbol;
	QString m_token;
	t_sr_threshold data;
	int counter = 0;

	xml.readNext();

	while (++counter < 1000 && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toLower() == TOKEN_THRESHOLD))
	{
		if (xml.tokenType() == QXmlStreamReader::StartElement)
		{
			if (xml.name() == "market")
			{
				symbol = xml.attributes().value("symbol").toString();
				config_properties[TOKEN_RANK_LASTDATE] = xml.attributes().value(TOKEN_RANK_LASTDATE).toString();

				int counter_2 = 0;

				while (++counter_2 < 3000 && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "market"))
				{
					xml.readNext();

					if (xml.tokenType() == QXmlStreamReader::StartElement)
					{
						m_token = xml.name().toString().toLower();

						if (m_token == "sr_threshold")
						{
							for (int i = 1; i <= MAX_SUPPORT_RESISTANCE_THRESHOLD; ++i)
							{
								int counter_3 = 0;
								while (++counter_3 < 100 && !xml.readNextStartElement()) /** do nothing **/;

								if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == "param")
								{
									data.t_id = xml.attributes().value("index").toInt();
									data.test_point = xml.attributes().value("test_point").toInt();
									data.break_threshold = xml.attributes().value("break_threshold").toDouble();
									data.react_threshold = data.break_threshold;
									data.recalculate = (xml.attributes().value("recalculate").toInt() == 1);

									if (list_sr_threshold.contains(symbol))
									{
										thres = list_sr_threshold[symbol];
									}

									thres.push_back(data);
									list_sr_threshold.insert(symbol , thres);
									thres.clear();
								}
							}
						}

						m_token = xml.name().toString().toLower();

						if (m_token == TOKEN_CGF || m_token == TOKEN_CLF || m_token == TOKEN_CGS || m_token == TOKEN_CLS ||
								m_token == TOKEN_DISTFSCROSS || m_token == TOKEN_FGS || m_token == TOKEN_FLS)
						{
							xml_parse_close_fs (xml, symbol, m_token);
						}
					}
				}
			}
		}

		xml.readNext();
	}
}

void XmlConfigHandler::xml_parse_symbol (QXmlStreamReader &xml, QStringList *out)
{
	int counter = 0;
	xml.readNext();

	while (++counter < 100 && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toLower() == TOKEN_SYMBOL))
	{
		if (xml.tokenType() == QXmlStreamReader::StartElement)
		{
			if (xml.name().toString().toLower() == "item")
			{
				xml.readNext();
				out->push_back(xml.text().toString());
			}
		}

		xml.readNext();
	}
}

void XmlConfigHandler::xml_parse_threshold_1 (QXmlStreamReader &xml, QVector<t_threshold_1> *out, const QString &token_name)
{
	/**
	 * We use c_limit in here to tackle problem, when there's corrupt xml format.
	 * For example, we found <lookahead> open token, there's no </lookahead> in xml file.
	 * This situation will lead to infinite loop.
	 * So we introduce c_limit, to allow function terminate after certain iteration.
	 **/
	int counter = 0;
	xml.readNext();

	while (++counter < 150 && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toLower() == token_name))
	{
		if (xml.tokenType() == QXmlStreamReader::StartElement)
		{
			if (xml.name() == "item")
			{
				t_threshold_1 m_data;
				m_data.t_id = xml.attributes().value("id").toInt();
				m_data.operator1 = xml.attributes().value("operator1").toString();
				m_data.operator2 = xml.attributes().value("operator2").toString();
				m_data.value1 = xml.attributes().value("value1").toDouble();
				m_data.value2 = xml.attributes().value("value2").toDouble();
				out->push_back(m_data);
			}
		}

		xml.readNext();
	}
}

void XmlConfigHandler::xml_parse_lookahead (QXmlStreamReader &xml)
{
	/**
	 * We use c_limit = 150 in here to tackle problem, when there's corrupt xml format.
	 * For example, we found <lookahead> open token, there's no </lookahead> in xml file.
	 * This situation will lead to infinite loop.
	 * So we introduce c_limit, to allow function terminate after certain iteration.
	 **/
	int counter = 0;

	xml.readNext();

	while (++counter < 150 && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toLower() == TOKEN_LOOKAHEAD)) {

		if (xml.tokenType() == QXmlStreamReader::StartElement) {
			if (xml.name().toString().toLower() == "item") {
				t_simple_keyvalue m_data;
				m_data.t_id = xml.attributes().value("id").toInt();
				m_data.value = xml.attributes().value("value").toDouble();
				t_lookahead.push_back(m_data);
			}
		}

		xml.readNext();
	}
}


//
// GETTER FUNCTION
//
QStringList XmlConfigHandler::get_list_symbol() const { return list_symbol; }
QMap<int, QString> XmlConfigHandler::get_interval_weight() const { return interval_weight; }
QMap<int, int> XmlConfigHandler::get_interval_threshold() const { return interval_threshold; }
QMap<QString, QVector<t_sr_threshold>> XmlConfigHandler::get_list_threshold() const { return list_sr_threshold;}
QVector<t_threshold_1> XmlConfigHandler::get_macd_threshold() const { return t_macd; }
QVector<t_threshold_1> XmlConfigHandler::get_rsi_threshold() const { return t_rsi; }
QVector<t_threshold_1> XmlConfigHandler::get_slowk_threshold() const { return t_slowk; }
QVector<t_threshold_1> XmlConfigHandler::get_slowd_threshold() const { return t_slowd; }
QMap<QString, QVector<t_threshold_2>> XmlConfigHandler::get_cgf_threshold() const { return t_cgf; }
QMap<QString, QVector<t_threshold_2>> XmlConfigHandler::get_clf_threshold() const { return t_clf; }
QMap<QString, QVector<t_threshold_2>> XmlConfigHandler::get_cgs_threshold() const { return t_cgs; }
QMap<QString, QVector<t_threshold_2>> XmlConfigHandler::get_cls_threshold() const { return t_cls; }
QMap<QString, QVector<t_threshold_2>> XmlConfigHandler::get_distfscross_threshold() const { return t_distfscross; }
QMap<QString, QVector<t_threshold_2>> XmlConfigHandler::get_fgs_threshold() const { return t_fgs; }
QMap<QString, QVector<t_threshold_2>> XmlConfigHandler::get_fls_threshold() const { return t_fls; }
QVector<t_threshold_2> XmlConfigHandler::get_cgf_threshold(const QString &symbol) const { return t_cgf[symbol]; }
QVector<t_threshold_2> XmlConfigHandler::get_clf_threshold(const QString &symbol) const { return t_clf[symbol]; }
QVector<t_threshold_2> XmlConfigHandler::get_cgs_threshold(const QString &symbol) const { return t_cgs[symbol]; }
QVector<t_threshold_2> XmlConfigHandler::get_cls_threshold(const QString &symbol) const { return t_cls[symbol]; }
QVector<t_threshold_2> XmlConfigHandler::get_distfscross_threshold(const QString &symbol) const { return t_distfscross[symbol]; }
QVector<t_threshold_2> XmlConfigHandler::get_fgs_threshold(const QString &symbol) const { return t_fgs[symbol]; }
QVector<t_threshold_2> XmlConfigHandler::get_fls_threshold(const QString &symbol) const { return t_fls[symbol]; }
QVector<t_simple_keyvalue> XmlConfigHandler::get_lookahead() const { return t_lookahead; }
QMap<QString,t_twsmapping> XmlConfigHandler::get_tws_mapping() const { return tws_mapping; }
t_twsmapping XmlConfigHandler::get_tws_mapping(const QString &symbol) const { return tws_mapping[symbol]; }

QDateTime XmlConfigHandler::get_last_updated() const
{
	return QDateTime::fromString(config_properties[TOKEN_LAST_UPDATE], "yyyy/MM/dd hh:mm::ss");
}

QString XmlConfigHandler::get_input_dir() const
{
	return config_properties[TOKEN_INPUT_DIR];
}

QString XmlConfigHandler::get_result_dir() const
{
	return config_properties[TOKEN_RESULT_DIR];
}

QString XmlConfigHandler::get_database_dir() const
{
	return config_properties[TOKEN_DATABASE_DIR];
}

QString XmlConfigHandler::get_ts_strategy_dir() const
{
	return config_properties[TOKEN_TS_STRATEGY_DIR];
}

QString XmlConfigHandler::get_searchpattern_dir() const
{
	return config_properties[TOKEN_SEARCHPATTERN_DIR];
}

QString XmlConfigHandler::get_ipc_name() const
{
	return config_properties[TOKEN_IPCNAME];
}

double XmlConfigHandler::get_initial_capital() const
{
	return config_properties[TOKEN_INITIAL_CAPITAL].toDouble();
}

double XmlConfigHandler::get_timer_interval() const
{
	return config_properties[TOKEN_TIMER_INTERVAL].toDouble();
}

int XmlConfigHandler::get_reduce_number() const
{
	return config_properties[TOKEN_REDUCE_NUMBER].toInt();
}

QString XmlConfigHandler::get_tws_id() const
{
	return config_properties[TOKEN_TWS_ID];
}

QString XmlConfigHandler::get_tws_ip() const
{
	return config_properties[TOKEN_TWS_IP];
}

QString XmlConfigHandler::get_tws_port() const
{
	return config_properties[TOKEN_TWS_PORT];
}

QString XmlConfigHandler::get_rank_lastdate() const
{
	return config_properties[TOKEN_RANK_LASTDATE];
}


//
// SETTER FUNCTION
//
void XmlConfigHandler::set_last_updated (const QDateTime &value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_LAST_UPDATE] = value.toString("yyyy/MM/dd hh:mm:ss");
}

void XmlConfigHandler::set_input_dir (const QString &value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_INPUT_DIR] = value;
}

void XmlConfigHandler::set_result_dir (const QString &value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_RESULT_DIR] = value;
}

void XmlConfigHandler::set_database_dir (const QString &value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_DATABASE_DIR] = value;
}

void XmlConfigHandler::set_ts_strategy_dir (const QString &value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_TS_STRATEGY_DIR] = value;
}

void XmlConfigHandler::set_searchpattern_dir (const QString &value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_SEARCHPATTERN_DIR] = value;
}

void XmlConfigHandler::set_ipc_name (const QString &value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_IPCNAME] = value;
}

void XmlConfigHandler::set_inital_capital (double value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_INITIAL_CAPITAL] = QString::number(value,'f');
}

void XmlConfigHandler::set_timer_interval (double value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_TIMER_INTERVAL] = QString::number(value,'f');
}

void XmlConfigHandler::set_reduce_number (int value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_REDUCE_NUMBER] = QString::number(value);
}

void XmlConfigHandler::set_tws_id (const QString &value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_TWS_ID] = value;
}

void XmlConfigHandler::set_tws_ip (const QString &value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_TWS_IP] = value;
}

void XmlConfigHandler::set_tws_port (const QString &value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_TWS_PORT] = value;
}

void XmlConfigHandler::set_rank_lastdate(const QString &value)
{
	QMutexLocker locker(&_mutex);
	config_properties[TOKEN_RANK_LASTDATE] = value;
}

void XmlConfigHandler::write_config_default()
{
	// we don't need put mutex here, there's no modification on cricital data

	QFile file(QCoreApplication::applicationDirPath() + "/" + config_filename);

	if (file.open(QFile::WriteOnly))
	{
		QVector<std::pair<QString,QString>> sr_threshold;

		sr_threshold.append(std::make_pair("@AD", "0.001"));
		sr_threshold.append(std::make_pair("@AD", "0"));
		sr_threshold.append(std::make_pair("@AD", "0"));
		sr_threshold.append(std::make_pair("@AD", "0"));
		sr_threshold.append(std::make_pair("@AD", "0"));

		sr_threshold.append(std::make_pair("@CL", "0.2"));
		sr_threshold.append(std::make_pair("@CL", "0.1"));
		sr_threshold.append(std::make_pair("@CL", "0"));
		sr_threshold.append(std::make_pair("@CL", "0"));
		sr_threshold.append(std::make_pair("@CL", "0"));

		sr_threshold.append(std::make_pair("@EC", "0.001"));
		sr_threshold.append(std::make_pair("@EC", "0"));
		sr_threshold.append(std::make_pair("@EC", "0"));
		sr_threshold.append(std::make_pair("@EC", "0"));
		sr_threshold.append(std::make_pair("@EC", "0"));

		sr_threshold.append(std::make_pair("@ES", "1"));
		sr_threshold.append(std::make_pair("@ES", "2"));
		sr_threshold.append(std::make_pair("@ES", "0"));
		sr_threshold.append(std::make_pair("@ES", "0"));
		sr_threshold.append(std::make_pair("@ES", "0"));

		sr_threshold.append(std::make_pair("@GC", "1"));
		sr_threshold.append(std::make_pair("@GC", "0"));
		sr_threshold.append(std::make_pair("@GC", "0"));
		sr_threshold.append(std::make_pair("@GC", "0"));
		sr_threshold.append(std::make_pair("@GC", "0"));

		sr_threshold.append(std::make_pair("@JY", "0.001"));
		sr_threshold.append(std::make_pair("@JY", "0"));
		sr_threshold.append(std::make_pair("@JY", "0"));
		sr_threshold.append(std::make_pair("@JY", "0"));
		sr_threshold.append(std::make_pair("@JY", "0"));

		sr_threshold.append(std::make_pair("@NIY", "20"));
		sr_threshold.append(std::make_pair("@NIY", "10"));
		sr_threshold.append(std::make_pair("@NIY", "0"));
		sr_threshold.append(std::make_pair("@NIY", "0"));
		sr_threshold.append(std::make_pair("@NIY", "0"));

		sr_threshold.append(std::make_pair("@NQ", "2"));
		sr_threshold.append(std::make_pair("@NQ", "4"));
		sr_threshold.append(std::make_pair("@NQ", "0"));
		sr_threshold.append(std::make_pair("@NQ", "0"));
		sr_threshold.append(std::make_pair("@NQ", "0"));

		sr_threshold.append(std::make_pair("HHI", "10"));
		sr_threshold.append(std::make_pair("HHI", "20"));
		sr_threshold.append(std::make_pair("HHI", "40"));
		sr_threshold.append(std::make_pair("HHI", "0"));
		sr_threshold.append(std::make_pair("HHI", "0"));

		sr_threshold.append(std::make_pair("HSI", "20"));
		sr_threshold.append(std::make_pair("HSI", "10"));
		sr_threshold.append(std::make_pair("HSI", "40"));
		sr_threshold.append(std::make_pair("HSI", "0"));
		sr_threshold.append(std::make_pair("HSI", "0"));

		sr_threshold.append(std::make_pair("IFB", "4"));
		sr_threshold.append(std::make_pair("IFB", "0"));
		sr_threshold.append(std::make_pair("IFB", "0"));
		sr_threshold.append(std::make_pair("IFB", "0"));
		sr_threshold.append(std::make_pair("IFB", "0"));

		sr_threshold.append(std::make_pair("@SI", "0.01"));
		sr_threshold.append(std::make_pair("@SI", "0.02"));
		sr_threshold.append(std::make_pair("@SI", "0"));
		sr_threshold.append(std::make_pair("@SI", "0"));
		sr_threshold.append(std::make_pair("@SI", "0"));

		QXmlStreamWriter stream(&file);
		stream.setAutoFormatting(true);
		stream.writeStartDocument();
		stream.writeStartElement("Application");
		stream.writeAttribute("name", "TS Finance");
		stream.writeAttribute("version", QCoreApplication::applicationVersion());

		appendXmlElement(&stream, TOKEN_LAST_UPDATE, "");
		appendXmlElement(&stream, TOKEN_DATABASE_DIR, DEFAULT_DATABASE_DIR);
		appendXmlElement(&stream, TOKEN_RESULT_DIR, DEFAULT_RESULT_DIR);
		appendXmlElement(&stream, TOKEN_INPUT_DIR, DEFAULT_INPUT_DIR);
		appendXmlElement(&stream, TOKEN_PROCESS_DIR, DEFAULT_PROCESS_DIR);
		appendXmlElement(&stream, TOKEN_TS_STRATEGY_DIR, DEFAULT_STRATEGY_DIR);
		appendXmlElement(&stream, TOKEN_SEARCHPATTERN_DIR, DEFAULT_SEARCHPATTERN_DIR);
		appendXmlElement(&stream, TOKEN_IPCNAME, "dbupdater");
		appendXmlElement(&stream, TOKEN_UPDATE_PARENT_INDEX , "1");
		appendXmlElement(&stream, TOKEN_UPDATE_SUPPORT_RESISTANCE, "1");
		appendXmlElement(&stream, TOKEN_UPDATE_HISTOGRAM, "1");
		appendXmlElement(&stream, TOKEN_DEVELOPER_MODE, "0");
		appendXmlElement(&stream, TOKEN_INITIAL_CAPITAL, "1000000");
		appendXmlElement(&stream, TOKEN_REDUCE_NUMBER, "1");
		appendXmlElement(&stream, TOKEN_TIMER_INTERVAL, "5");

		begin_TWS_XMLElement(&stream, "127.0.0.1", "7496", "0");
		append_TWS_mapping(&stream, "@AD",  "6AH6",   "FUT", "GLOBEX");
		append_TWS_mapping(&stream, "@CL",  "CLG6",   "FUT", "NYMEX");
		append_TWS_mapping(&stream, "@EC",  "6EH6",   "FUT", "GLOBEX");
		append_TWS_mapping(&stream, "@ES",  "ESH6",   "FUT", "GLOBEX");
		append_TWS_mapping(&stream, "@GC",  "GCF6",   "FUT", "NYMEX");
		append_TWS_mapping(&stream, "@JY",  "6JH6",   "FUT", "GLOBEX");
		append_TWS_mapping(&stream, "@NIY", "NIYH6",  "FUT", "GLOBEX");
		append_TWS_mapping(&stream, "@NQ",  "NQH6",   "FUT", "GLOBEX");
		append_TWS_mapping(&stream, "HHI",  "HHIF16", "FUT", "HKFE");
		append_TWS_mapping(&stream, "HSI",  "HSIF16", "FUT", "HKFE");
		end_TWS_XMLElement(&stream);

		stream.writeStartElement(TOKEN_SYMBOL);
		stream.writeTextElement("item","@AD");
		stream.writeTextElement("item","@CL");
		stream.writeTextElement("item","@EC");
		stream.writeTextElement("item","@ES");
		stream.writeTextElement("item","@GC");
		stream.writeTextElement("item","@JY");
		stream.writeTextElement("item","@NIY");
		stream.writeTextElement("item","@NQ");
		stream.writeTextElement("item","HHI");
		stream.writeTextElement("item","HSI");
		stream.writeTextElement("item","IFB");
		stream.writeTextElement("item","@SI");
		stream.writeEndElement();

		stream.writeStartElement(TOKEN_MACD);
		appendXmlElement_Threshold_1(&stream, "1", "<","0", ">", "0");
		appendXmlElement_Threshold_1(&stream, "2", "","", "", "");
		appendXmlElement_Threshold_1(&stream, "3", "","", "", "");
		stream.writeEndElement();

		stream.writeStartElement(TOKEN_RSI);
		appendXmlElement_Threshold_1(&stream, "1", "<", "30", ">", "70");
		appendXmlElement_Threshold_1(&stream, "2", "<", "35", ">", "65");
		appendXmlElement_Threshold_1(&stream, "3", "<", "40", ">", "60");
		stream.writeEndElement();

		stream.writeStartElement(TOKEN_SLOWK);
		appendXmlElement_Threshold_1(&stream, "1", "<", "20", ">", "80");
		appendXmlElement_Threshold_1(&stream, "2", "<", "30", ">", "70");
		appendXmlElement_Threshold_1(&stream, "3", "<", "40", ">", "60");
//    appendXmlElement_Threshold_1(&stream, "2", "<", "25", ">", "75"); // old
		stream.writeEndElement();

		stream.writeStartElement(TOKEN_SLOWD);
		appendXmlElement_Threshold_1(&stream, "1", "<", "20", ">", "80");
		appendXmlElement_Threshold_1(&stream, "2", "<", "30", ">", "70");
		appendXmlElement_Threshold_1(&stream, "3", "<", "40", ">", "60");
//    appendXmlElement_Threshold_1(&stream, "2", "<", "25", ">", "75"); // old
		stream.writeEndElement();

		stream.writeStartElement(TOKEN_LOOKAHEAD);
		appendXmlElement_Threshold_2(&stream, "0", "1");
		appendXmlElement_Threshold_2(&stream, "1", "3");
		appendXmlElement_Threshold_2(&stream, "2", "5");
		appendXmlElement_Threshold_2(&stream, "3", "10");
		appendXmlElement_Threshold_2(&stream, "4", "20");
		appendXmlElement_Threshold_2(&stream, "5", "50");
		stream.writeEndElement();

		// intervals (use minute as weight measurement unit)
		stream.writeStartElement(TOKEN_INTERVAL);
		appendXmlElement_Interval(&stream, "1min", "1", "0");
		appendXmlElement_Interval(&stream, "5min", "5", "0");
		appendXmlElement_Interval(&stream, "60min", "60", "0");
		appendXmlElement_Interval(&stream, "Daily", "1440", "0");
		appendXmlElement_Interval(&stream, "Weekly", "10080", "0");
		appendXmlElement_Interval(&stream, "Monthly", "43200", "0");
		stream.writeEndElement();

		// threshold parameters
		QStringList group_token_threshold;
		QString prev_symbol = "";

		stream.writeStartElement("thresholds");
		group_token_threshold.push_back(TOKEN_CGF);
		group_token_threshold.push_back(TOKEN_CLF);
		group_token_threshold.push_back(TOKEN_CGS);
		group_token_threshold.push_back(TOKEN_CLS);
		group_token_threshold.push_back(TOKEN_DISTFSCROSS);
		group_token_threshold.push_back(TOKEN_FGS);
		group_token_threshold.push_back(TOKEN_FLS);

		for (int i = 0; i < sr_threshold.size(); ++i)
		{
			if (!prev_symbol.isEmpty() && prev_symbol != sr_threshold[i].first)
			{
				stream.writeEndElement(); // end market
			}

			prev_symbol = sr_threshold[i].first;
			stream.writeComment(sr_threshold[i].first);
			stream.writeStartElement("market");
			stream.writeAttribute("symbol", sr_threshold[i].first);
			stream.writeAttribute(TOKEN_RANK_LASTDATE, "2015-12-31");
			stream.writeStartElement("sr_threshold");

			for (int j = 0; i < sr_threshold.size() && j < MAX_SUPPORT_RESISTANCE_THRESHOLD; ++j)
			{
				appendXmlElement_SupportResistance_V2(&stream, QString::number(j), "2", sr_threshold[i].second, "0");
				++i;
			}

			--i;
			stream.writeEndElement(); // end sr_threshold

			QVector<double> d = lookup_close_threshold(prev_symbol);

			// cgf, clf, cgs, cls, dist fs-cross, fgs, fls
			for (int x = 0; x < group_token_threshold.size(); ++x)
			{
				stream.writeStartElement(group_token_threshold[x]);

				for (int z = 0 ; z < MAX_CLOSE_THRESHOLD; ++z)
				{
					if (z < d.size())
						appendXmlElement_Threshold_2(&stream, QString::number(z), QString::number(d[z],'f',4));
					else
						appendXmlElement_Threshold_2(&stream, QString::number(z), "");
				}
				stream.writeEndElement();
			}
		}

		stream.writeEndElement(); // end market (last)
		stream.writeEndElement(); // end threshold
		stream.writeEndElement(); // end application
		stream.writeEndDocument();
	}

	file.close();
}

void XmlConfigHandler::write_config()
{
	QMutexLocker locker(&_mutex);

	QString config_path = QCoreApplication::applicationDirPath() + "/" + config_filename;
	QFile file(config_path);
	QFile::remove(config_path);

	if (file.open(QFile::ReadWrite))
	{
		QXmlStreamWriter stream(&file);
		stream.setAutoFormatting(true);
		stream.writeStartDocument();
		stream.writeStartElement("Application");
		stream.writeAttribute("name", "TS Finance");
		stream.writeAttribute("version", QCoreApplication::applicationVersion());

		appendXmlElement(&stream, TOKEN_LAST_UPDATE, config_properties[TOKEN_LAST_UPDATE]);
		appendXmlElement(&stream, TOKEN_DATABASE_DIR, config_properties[TOKEN_DATABASE_DIR]);
		appendXmlElement(&stream, TOKEN_RESULT_DIR, config_properties[TOKEN_RESULT_DIR]);
		appendXmlElement(&stream, TOKEN_INPUT_DIR, config_properties[TOKEN_INPUT_DIR]);
		appendXmlElement(&stream, TOKEN_PROCESS_DIR, config_properties[TOKEN_PROCESS_DIR]);
		appendXmlElement(&stream, TOKEN_TS_STRATEGY_DIR, config_properties[TOKEN_TS_STRATEGY_DIR]);
		appendXmlElement(&stream, TOKEN_SEARCHPATTERN_DIR, config_properties[TOKEN_SEARCHPATTERN_DIR]);
		appendXmlElement(&stream, TOKEN_IPCNAME, config_properties[TOKEN_IPCNAME]);
		appendXmlElement(&stream, TOKEN_UPDATE_PARENT_INDEX, enable_parent_indexing? "1":"0");
		appendXmlElement(&stream, TOKEN_UPDATE_SUPPORT_RESISTANCE, enable_update_support_resistance? "1":"0");
		appendXmlElement(&stream, TOKEN_UPDATE_HISTOGRAM, enable_update_histogram? "1":"0");
		appendXmlElement(&stream, TOKEN_DEVELOPER_MODE, enable_developer_mode? "1":"0");
		appendXmlElement(&stream, TOKEN_INITIAL_CAPITAL, config_properties[TOKEN_INITIAL_CAPITAL]);
		appendXmlElement(&stream, TOKEN_REDUCE_NUMBER, config_properties[TOKEN_REDUCE_NUMBER]);
		appendXmlElement(&stream, TOKEN_TIMER_INTERVAL, config_properties[TOKEN_TIMER_INTERVAL]);

		begin_TWS_XMLElement(&stream, config_properties[TOKEN_TWS_IP],
																	config_properties[TOKEN_TWS_PORT],
																	config_properties[TOKEN_TWS_ID]);

		for (auto it_mapping = tws_mapping.begin(); it_mapping != tws_mapping.end(); ++it_mapping)
		{
			t_twsmapping data = it_mapping.value();
			append_TWS_mapping(&stream, it_mapping.key(), data.local_symbol, data.type, data.exchange);
		}

		end_TWS_XMLElement(&stream);

		stream.writeStartElement(TOKEN_SYMBOL);
		for (int i = 0; i < list_symbol.length(); ++i)
		{
			stream.writeTextElement("item", list_symbol[i]);
		}
		stream.writeEndElement();

		stream.writeStartElement(TOKEN_MACD);
		for (int i = 0; i < t_macd.size(); ++i)
		{
			appendXmlElement_Threshold_1(&stream, QString::number(t_macd[i].t_id),
																	 t_macd[i].operator1, QString::number(t_macd[i].value1),
																	 t_macd[i].operator2, QString::number(t_macd[i].value2));
		}
		stream.writeEndElement();

		stream.writeStartElement(TOKEN_RSI);
		for (int i = 0; i < t_rsi.size(); ++i)
		{
			appendXmlElement_Threshold_1(&stream, QString::number(t_rsi[i].t_id),
																	 t_rsi[i].operator1, QString::number(t_rsi[i].value1),
																	 t_rsi[i].operator2, QString::number(t_rsi[i].value2));
		}
		stream.writeEndElement();

		stream.writeStartElement(TOKEN_SLOWK);
		for (int i = 0; i < t_slowk.size(); ++i)
		{
			appendXmlElement_Threshold_1(&stream, QString::number(t_slowk[i].t_id),
																	 t_slowk[i].operator1, QString::number(t_slowk[i].value1),
																	 t_slowk[i].operator2, QString::number(t_slowk[i].value2));
		}
		stream.writeEndElement();

		stream.writeStartElement(TOKEN_SLOWD);
		for (int i = 0; i < t_slowd.size(); ++i)
		{
			appendXmlElement_Threshold_1(&stream, QString::number(t_slowd[i].t_id),
																	 t_slowd[i].operator1, QString::number(t_slowd[i].value1),
																	 t_slowd[i].operator2, QString::number(t_slowd[i].value2));
		}
		stream.writeEndElement();

		stream.writeStartElement(TOKEN_LOOKAHEAD);
		for (int i = 0; i < t_lookahead.size(); ++i)
		{
			appendXmlElement_Threshold_2(&stream, QString::number(t_lookahead[i].t_id), QString::number(t_lookahead[i].value));
		}
		stream.writeEndElement();


		stream.writeStartElement(TOKEN_INTERVAL);

		for (auto it = interval_weight.begin(); it != interval_weight.end(); ++it)
		{
			appendXmlElement_Interval(&stream, it.value(), QString::number(it.key()), QString::number(interval_threshold[it.key()]));
		}
		stream.writeEndElement();

		QVector<t_sr_threshold> data;
		stream.writeStartElement("thresholds");

		for (auto it = list_sr_threshold.begin(); it != list_sr_threshold.end(); ++it)
		{
			data = it.value();
			stream.writeComment(it.key());
			stream.writeStartElement("market");
			stream.writeAttribute("symbol", it.key());
			stream.writeAttribute(TOKEN_RANK_LASTDATE, "2015-12-31");
			stream.writeStartElement("sr_threshold");

			for (int j = 0; j < data.size(); ++j)
			{
				appendXmlElement_SupportResistance_V2(
					&stream, QString::number(j),
					QString::number(data[j].test_point),
					QString::number(data[j].break_threshold,'f',4),
					data[j].recalculate? "1":"0");
			}

			stream.writeEndElement(); // end sr threshold

			stream.writeStartElement (TOKEN_CGF);
			for (int j = 0; j < t_cgf[it.key()].size(); ++j)
			{
				appendXmlElement_Threshold_2 (&stream, QString::number(t_cgf[it.key()][j].t_id), t_cgf[it.key()][j].value);
			}
			stream.writeEndElement();

			stream.writeStartElement (TOKEN_CLF);
			for (int j = 0; j < t_clf[it.key()].size(); ++j)
			{
				appendXmlElement_Threshold_2 (&stream, QString::number(t_clf[it.key()][j].t_id), t_clf[it.key()][j].value);
			}
			stream.writeEndElement();

			stream.writeStartElement (TOKEN_CGS);
			for (int j = 0; j < t_cgs[it.key()].size(); ++j)
			{
				appendXmlElement_Threshold_2 (&stream, QString::number(t_cgs[it.key()][j].t_id), t_cgs[it.key()][j].value);
			}
			stream.writeEndElement();

			stream.writeStartElement (TOKEN_CLS);
			for (int j = 0; j < t_cls[it.key()].size(); ++j)
			{
				appendXmlElement_Threshold_2 (&stream, QString::number(t_cls[it.key()][j].t_id), t_cls[it.key()][j].value);
			}
			stream.writeEndElement();

			stream.writeStartElement (TOKEN_DISTFSCROSS);
			for (int j = 0; j < t_distfscross[it.key()].size(); ++j)
			{
				appendXmlElement_Threshold_2 (&stream, QString::number(t_distfscross[it.key()][j].t_id), t_distfscross[it.key()][j].value);
			}
			stream.writeEndElement();

			stream.writeStartElement (TOKEN_FGS);
			for (int j = 0; j < t_fgs[it.key()].size(); ++j)
			{
				appendXmlElement_Threshold_2 (&stream, QString::number(t_fgs[it.key()][j].t_id), t_fgs[it.key()][j].value);
			}
			stream.writeEndElement();

			stream.writeStartElement (TOKEN_FLS);
			for (int j = 0; j < t_fls[it.key()].size(); ++j)
			{
				appendXmlElement_Threshold_2 (&stream, QString::number(t_fls[it.key()][j].t_id), t_fls[it.key()][j].value);
			}
			stream.writeEndElement();

			stream.writeEndElement(); // end market
		}

		stream.writeEndElement(); // end threshold
		stream.writeEndElement(); // end application
		stream.writeEndDocument();
	}

	file.close();
}

void XmlConfigHandler::read_config()
{
	QMutexLocker locker(&_mutex);

	QFile file(QCoreApplication::applicationDirPath() + "/" + config_filename);

	initialize_variable();

	// create new config, if not exists
	if (!file.exists())
	{
		write_config_default();
	}

	if (file.open(QFile::ReadOnly))
	{
		QXmlStreamReader xml(&file);
		QXmlStreamReader::TokenType token;
		QString token_name;

		while (!xml.atEnd() && !xml.hasError())
		{
			token = xml.readNext();

			if (token == QXmlStreamReader::StartDocument) continue;

			if (token == QXmlStreamReader::StartElement)
			{
				token_name = xml.name().toString().toLower(); // just in case

				if (token_name == TOKEN_INPUT_DIR) config_properties[TOKEN_INPUT_DIR] = _XML_VALUE;
				else if (token_name == TOKEN_RESULT_DIR) config_properties[TOKEN_RESULT_DIR] = _XML_VALUE;
				else if (token_name == TOKEN_DATABASE_DIR)
				{
					QString dir_path = _XML_VALUE;
					config_properties[TOKEN_DATABASE_DIR] = dir_path.isEmpty() ? QCoreApplication::applicationDirPath() : dir_path;
				}
				else if (token_name == TOKEN_PROCESS_DIR) config_properties[TOKEN_PROCESS_DIR] = _XML_VALUE;
				else if (token_name == TOKEN_TS_STRATEGY_DIR) config_properties[TOKEN_TS_STRATEGY_DIR] = _XML_VALUE;
				else if (token_name == TOKEN_SEARCHPATTERN_DIR) config_properties[TOKEN_SEARCHPATTERN_DIR] = _XML_VALUE;
				else if (token_name == TOKEN_IPCNAME) config_properties[TOKEN_IPCNAME] = _XML_VALUE;
				else if (token_name == TOKEN_LAST_UPDATE) config_properties[TOKEN_LAST_UPDATE] = _XML_VALUE;
				else if (token_name == TOKEN_INITIAL_CAPITAL) config_properties[TOKEN_INITIAL_CAPITAL] = _XML_VALUE;
				else if (token_name == TOKEN_REDUCE_NUMBER) config_properties[TOKEN_REDUCE_NUMBER] = _XML_VALUE;
				else if (token_name == TOKEN_TIMER_INTERVAL) config_properties[TOKEN_TIMER_INTERVAL] = _XML_VALUE;
				else if (token_name == TOKEN_UPDATE_PARENT_INDEX) enable_parent_indexing = (_XML_VALUE_INT == 1);
				else if (token_name == TOKEN_UPDATE_SUPPORT_RESISTANCE) enable_update_support_resistance = (_XML_VALUE_INT == 1);
				else if (token_name == TOKEN_UPDATE_HISTOGRAM) enable_update_histogram = (_XML_VALUE_INT == 1);
				else if (token_name == TOKEN_DEVELOPER_MODE) enable_developer_mode = (_XML_VALUE_INT == 1);
				else if (token_name == TOKEN_TWS)
				{
					config_properties[TOKEN_TWS_ID] = _XML_TWS_ID;
					config_properties[TOKEN_TWS_PORT] = _XML_TWS_PORT;
					config_properties[TOKEN_TWS_IP] = _XML_TWS_IP;

					int counter = 0;
					xml.readNext();

					while (++counter < 300 && !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toLower() == TOKEN_TWS))
					{
						if (xml.tokenType() == QXmlStreamReader::StartElement)
						{
							if (xml.name().toString().toLower() == TOKEN_TWS_MAPPING)
							{
								t_twsmapping data;
								data.local_symbol = xml.attributes().value(TOKEN_TWS_LOCAL_SYMBOL).toString();
								data.type = xml.attributes().value(TOKEN_TWS_TYPE).toString();
								data.exchange = xml.attributes().value(TOKEN_TWS_EXCHANGE).toString();
								tws_mapping.insert(xml.attributes().value(TOKEN_TWS_SYMBOL).toString(), data);
							}
						}
						xml.readNext();
					}
				}
				else if (token_name == TOKEN_SYMBOL) xml_parse_symbol(xml, &list_symbol);
				else if (token_name == TOKEN_INTERVAL) xml_parse_interval(xml);
				else if (token_name == TOKEN_MACD) xml_parse_threshold_1(xml, &t_macd, TOKEN_MACD);
				else if (token_name == TOKEN_RSI) xml_parse_threshold_1(xml, &t_rsi, TOKEN_RSI);
				else if (token_name == TOKEN_SLOWK) xml_parse_threshold_1(xml, &t_slowk, TOKEN_SLOWK);
				else if (token_name == TOKEN_SLOWD) xml_parse_threshold_1(xml, &t_slowd, TOKEN_SLOWD);
				else if (token_name == TOKEN_LOOKAHEAD) xml_parse_lookahead(xml);
				else if (token_name == TOKEN_THRESHOLD) xml_parse_threshold(xml);
			}
		}

		xml.clear();

		file.close();
	}
}

void XmlConfigHandler::reset_recalculate_threshold()
{
	QVector<t_sr_threshold> vec;

	for (auto it = list_sr_threshold.begin(); it != list_sr_threshold.end(); ++it)
	{
		vec = it.value();
		for (int i = 0; i < vec.length(); ++i)
		{
			vec[i].recalculate = false;
		}
		list_sr_threshold[it.key()] = vec;
	}
}
