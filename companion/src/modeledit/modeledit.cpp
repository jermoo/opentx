#include "modeledit.h"
#include "ui_modeledit.h"
#include "setup.h"
#include "heli.h"
#include "flightmodes.h"
#include "inputs.h"
#include "mixes.h"
#include "channels.h"
#include "curves.h"
#include "customswitches.h"
#include "customfunctions.h"
#include "telemetry.h"
#include <QScrollArea>

ModelEdit::ModelEdit(RadioData & radioData, int modelId, bool openWizard, bool isNew, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ModelEdit),
  modelId(modelId),
  model(radioData.models[modelId]),
  generalSettings(generalSettings)
{
  ui->setupUi(this);
  addTab(new Setup(this, model), tr("Setup"));
  addTab(new HeliPanel(this, model), tr("Heli"));
  addTab(new FlightModes(this, model, radioData.generalSettings), tr("Flight Modes"));
  addTab(new InputsPanel(this, model, radioData.generalSettings), tr("Inputs"));
  addTab(new MixesPanel(this, model, radioData.generalSettings), tr("Mixes"));
  addTab(new Channels(this, model), tr("Channels"));
  addTab(new CustomSwitchesPanel(this, model), tr("Custom Switches"));
  if (GetEepromInterface()->getCapability(CustomFunctions))
    addTab(new CustomFunctionsPanel(this, model, radioData.generalSettings), tr("Assignable Functions"));
  addTab(new Curves(this, model), tr("Curves"));
  // TODO remove this capability if (!GetEepromInterface()->getCapability(FSSwitch))
  if (GetEepromInterface()->getCapability(Telemetry) & TM_HASTELEMETRY)
    addTab(new TelemetryPanel(this, model), tr("Telemetry"));
}

ModelEdit::~ModelEdit()
{
  delete ui;
}

class VerticalScrollArea : public QScrollArea
{
  public:
    VerticalScrollArea(QWidget * parent, ModelPanel * panel);

  protected:
    virtual bool eventFilter(QObject *o, QEvent *e);

  private:
    ModelPanel * panel;
    QWidget * parent;
};

VerticalScrollArea::VerticalScrollArea(QWidget * parent, ModelPanel * panel):
  QScrollArea(parent),
  panel(panel),
  parent(parent)
{
  setWidgetResizable(true);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setWidget(panel);
  panel->installEventFilter(this);
}

bool VerticalScrollArea::eventFilter(QObject *o, QEvent *e)
{
  if (o == panel && e->type() == QEvent::Resize) {
    setMinimumWidth(panel->minimumSizeHint().width() + verticalScrollBar()->width());
  }
  return false;
}

void ModelEdit::addTab(ModelPanel *panel, QString text)
{
  panels << panel;
  QWidget * widget = new QWidget(ui->tabWidget);
  QVBoxLayout *baseLayout = new QVBoxLayout(widget);
  VerticalScrollArea * area = new VerticalScrollArea(widget, panel);
  baseLayout->addWidget(area);
  ui->tabWidget->addTab(widget, text);
  connect(panel, SIGNAL(modified()), this, SLOT(onTabModified()));
}

void ModelEdit::onTabModified()
{
  emit modified();
}

void ModelEdit::on_tabWidget_currentChanged(int index)
{
  panels[index]->update();
}

void ModelEdit::on_pushButton_clicked()
{
  launchSimulation();
}

// TODO merge both
#include "simulatordialog.h"
#include "xsimulatordialog.h"

void ModelEdit::launchSimulation()
{
  if (GetEepromInterface()->getSimulator()) {
    RadioData *simuData = new RadioData();
    simuData->generalSettings = generalSettings;
    simuData->models[modelId] = model;
    if (GetEepromInterface()->getCapability(SimulatorType)) {
      xsimulatorDialog sd(this);
      sd.loadParams(*simuData, modelId);
      sd.exec();
    }
    else {
      simulatorDialog sd(this);
      sd.loadParams(*simuData, modelId);
      sd.exec();
    }
    delete simuData;
  }
  else {
    QMessageBox::warning(NULL,
        QObject::tr("Warning"),
        QObject::tr("Simulator for this firmware is not yet available"));
  }
}

