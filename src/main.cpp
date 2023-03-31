#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QStyleFactory>
#include <QTranslator>


int main(int argc, char *argv[])
{
	/* Available styles: "Windows", "Fusion" */
	QApplication::setStyle(QStyleFactory::create("Fusion"));
	QIcon logoIcon = QIcon(":/resources/logo.ico");
	QApplication::setWindowIcon(logoIcon);
	QApplication app(argc, argv);

	QTranslator translator;
	const QStringList uiLanguages = QLocale::system().uiLanguages();
	for (const QString &locale:uiLanguages)
	{
		const QString baseName = "SupperCOM_" + QLocale(locale).name();
		if (translator.load(":/i18n/" + baseName))
		{
			QApplication::installTranslator(&translator);
			break;
		}
	}
	MainWindow w;
	w.show();
	return QApplication::exec();
}
