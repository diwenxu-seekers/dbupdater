// XmlConfigHandler.h
// Singleton class to maintain read/write to config.xml
#ifndef XMLCONFIGHANDLER
#define XMLCONFIGHANDLER

#pragma warning(push,3)
#include <QDateTime>
#include <QMap>
#include <QMutex>
#include <QVector>
#include <QXmlStreamReader>
#pragma warning(pop)

#include <memory>
#include <mutex>

// threshold for Support/Resistance
typedef struct {
	int t_id;
	QString symbol;
	int test_point; // 2
	double break_threshold;
	double react_threshold;
	bool recalculate;
} t_sr_threshold;

// threshold for MACD, RSI, SlowK, SlowD
typedef struct {
	int t_id;
	QString operator1;
	QString operator2;
	double value1;
	double value2;
} t_threshold_1;

// threshold for CGF, CLF, CGS, CLS, DistFS-Cross, FGS, FLS
typedef struct {
	int t_id;
	QString value; // allow to be null (not set)
} t_threshold_2;

// Simple KeyValue parameters
typedef struct {
	int  t_id;
	double value;
} t_simple_keyvalue;

// tws mapping
typedef struct {
	QString local_symbol;
	QString type;
	QString exchange;
} t_twsmapping;

class XmlConfigHandler {
	private:
		static std::shared_ptr<XmlConfigHandler> _instance;
		static std::once_flag _only_one;
		QString config_filename = "config.xml";
		QMutex _mutex;

		QMap<QString, QString> config_properties;
		QStringList list_symbol;
		QMap<int,QString> interval_weight;
		QMap<int,int> interval_threshold;
		QMap<QString, QVector<t_sr_threshold>> list_sr_threshold; // { Key = Symbol, Value = Thresholds }
		QVector<t_threshold_1> t_rsi;
		QVector<t_threshold_1> t_macd;
		QVector<t_threshold_1> t_slowk;
		QVector<t_threshold_1> t_slowd;
		QMap<QString, QVector<t_threshold_2>> t_cgf; // { Key = Symbol, Value = Thresholds }
		QMap<QString, QVector<t_threshold_2>> t_clf;
		QMap<QString, QVector<t_threshold_2>> t_cgs;
		QMap<QString, QVector<t_threshold_2>> t_cls;
		QMap<QString, QVector<t_threshold_2>> t_distfscross;
		QMap<QString, QVector<t_threshold_2>> t_fgs; // since v0.13
		QMap<QString, QVector<t_threshold_2>> t_fls; // since v0.13
		QVector<t_simple_keyvalue> t_lookahead;
		QMap<QString,t_twsmapping> tws_mapping; // since v0.14. mapping (symbol -> mapping_data)
		bool enable_developer_mode;
		bool enable_parent_indexing;
		bool enable_update_support_resistance;
		bool enable_update_histogram;

		XmlConfigHandler(){}
		XmlConfigHandler(XmlConfigHandler const&){}
		XmlConfigHandler& operator=(const XmlConfigHandler &handler)
		{
			if (this != &handler) _instance = handler._instance;
			return *this;
		}

		void initialize_variable();
		void xml_parse_interval(QXmlStreamReader &xml);
		void xml_parse_threshold(QXmlStreamReader &xml);
		void xml_parse_symbol(QXmlStreamReader &xml, QStringList *out);
		void xml_parse_threshold_1(QXmlStreamReader &xml, QVector<t_threshold_1> *out, const QString &token_name); // MACD, RSI, SlowK, SlowD
		void xml_parse_lookahead(QXmlStreamReader &xml);
		void xml_parse_close_fs(QXmlStreamReader &xml, const QString &symbol, const QString &token_name); // CGF, CLF, CGS, CLS, Dist(FS-Cross)
		void read_config();
		void write_config_default();

	public:
//		~XmlConfigHandler();
		void write_config();
		void reset_recalculate_threshold();

		static XmlConfigHandler *get_instance(bool readConfigFlag = false)
		{
			// build instance once
			std::call_once(XmlConfigHandler::_only_one,[]()
					{
						XmlConfigHandler::_instance.reset(new XmlConfigHandler());
						_instance->read_config();
					}
			);

			// allow developer to choose whether reload config values again or not
			// use in scenario where config.xml may be modified in middle of process
			if (readConfigFlag) {
				_instance->read_config();
			}

			return _instance.get();
		}

		void change_config_filename(const QString &filename) { config_filename = filename; }
		QString get_config_filename() const { return config_filename; }

		QDateTime get_last_updated() const;
		QString   get_input_dir() const ;
		QString   get_result_dir() const ;
		QString   get_database_dir() const ;
		QString   get_ts_strategy_dir() const;
		QString   get_searchpattern_dir() const;
		QString   get_ipc_name() const;
		double    get_initial_capital() const;
		double    get_timer_interval() const;
		int       get_reduce_number() const;
		QString   get_tws_id() const;
		QString   get_tws_port() const;
		QString   get_tws_ip() const;
		QString   get_rank_lastdate() const;

		void set_last_updated(const QDateTime &value);
		void set_input_dir(const QString &value);
		void set_result_dir(const QString &value);
		void set_database_dir(const QString &value);
		void set_ts_strategy_dir(const QString &value);
		void set_searchpattern_dir(const QString &value);
		void set_ipc_name(const QString &value);
		void set_inital_capital(double value);
		void set_timer_interval(double value);
		void set_reduce_number(int value);
		void set_tws_id(const QString &value);
		void set_tws_port(const QString &value);
		void set_tws_ip(const QString &value);
		void set_rank_lastdate(const QString &value);

		QStringList get_list_symbol() const;
		QMap<int, QString> get_interval_weight() const;
		QMap<int, int> get_interval_threshold() const;
		QMap<QString, QVector<t_sr_threshold>> get_list_threshold() const;
		QVector<t_threshold_1> get_macd_threshold() const;
		QVector<t_threshold_1> get_rsi_threshold() const;
		QVector<t_threshold_1> get_slowk_threshold() const;
		QVector<t_threshold_1> get_slowd_threshold() const;
		QMap<QString, QVector<t_threshold_2>> get_cgf_threshold() const;
		QMap<QString, QVector<t_threshold_2>> get_clf_threshold() const;
		QMap<QString, QVector<t_threshold_2>> get_cgs_threshold() const;
		QMap<QString, QVector<t_threshold_2>> get_cls_threshold() const;
		QMap<QString, QVector<t_threshold_2>> get_distfscross_threshold() const;
		QMap<QString, QVector<t_threshold_2>> get_fgs_threshold() const;
		QMap<QString, QVector<t_threshold_2>> get_fls_threshold() const;
		QVector<t_threshold_2> get_cgf_threshold(const QString &symbol) const;
		QVector<t_threshold_2> get_clf_threshold(const QString &symbol) const;
		QVector<t_threshold_2> get_cgs_threshold(const QString &symbol) const;
		QVector<t_threshold_2> get_cls_threshold(const QString &symbol) const;
		QVector<t_threshold_2> get_distfscross_threshold(const QString &symbol) const;
		QVector<t_threshold_2> get_fgs_threshold(const QString &symbol) const;
		QVector<t_threshold_2> get_fls_threshold(const QString &symbol) const;
		QVector<t_simple_keyvalue> get_lookahead() const;
		QMap<QString,t_twsmapping> get_tws_mapping() const;
		t_twsmapping get_tws_mapping(const QString &symbol) const;

		bool is_enable_developer_mode() const { return enable_developer_mode; }
		bool is_enable_parent_indexing() const { return enable_parent_indexing; }
		bool is_enable_update_support_resistance() const { return enable_update_support_resistance; }
		bool is_enable_update_histogram() const { return enable_update_histogram; }
		void set_list_symbol(const QStringList &symbols) { list_symbol = symbols; }
		void set_list_threshold(const QMap<QString, QVector<t_sr_threshold>> &threshold) { list_sr_threshold = threshold; }
		void set_interval_threshold(const QMap<int,int> &threshold) { interval_threshold = threshold; }
		void set_tws_mapping(const QMap<QString,t_twsmapping> &mapping) { tws_mapping = mapping; }
		void set_macd_threshold(const QVector<t_threshold_1> &t) { t_macd = t; }
		void set_rsi_threshold(const QVector<t_threshold_1> &t) { t_rsi = t; }
		void set_slowk_threshold(const QVector<t_threshold_1> &t) { t_slowk = t; }
		void set_slowd_threshold(const QVector<t_threshold_1> &t) { t_slowd = t; }
		void set_cgf_threshold(const QString &symbol, const QVector<t_threshold_2> &t) { t_cgf[symbol] = t; }
		void set_clf_threshold(const QString &symbol, const QVector<t_threshold_2> &t) { t_clf[symbol] = t; }
		void set_cgs_threshold(const QString &symbol, const QVector<t_threshold_2> &t) { t_cgs[symbol] = t; }
		void set_cls_threshold(const QString &symbol, const QVector<t_threshold_2> &t) { t_cls[symbol] = t; }
		void set_distfscross_threshold(const QString &symbol, const QVector<t_threshold_2> &t) { t_distfscross[symbol] = t; }
		void set_lookahead(const QVector<t_simple_keyvalue> &t) { t_lookahead = t; }
};

#endif // XMLCONFIGHANDLER
