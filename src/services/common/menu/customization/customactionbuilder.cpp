/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangyu<zhangyub@uniontech.com>
 *
 * Maintainer: zhangyu<zhangyub@uniontech.com>
 *             liqiang<liqianga@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "customactionbuilder.h"

#include "dfm-base/base/schemefactory.h"

#include <QDir>
#include <QUrl>
#include <QMenu>
#include <QFontMetrics>

DSC_BEGIN_NAMESPACE
DFMBASE_USE_NAMESPACE

CustomActionBuilder::CustomActionBuilder(QObject *parent)
    : QObject(parent), fm(QFontMetrics(QAction().font()))
{
}

QAction *CustomActionBuilder::buildAciton(const CustomActionData &actionData, QWidget *parentForSubmenu) const
{
    QAction *ret = nullptr;
    if (actionData.isAction()) {
        ret = createAciton(actionData);
    } else {
        ret = createMenu(actionData, parentForSubmenu);
    }

    return ret;
}

void CustomActionBuilder::setActiveDir(const QUrl &dir)
{
    dirPath = dir;
    QString errString;
    auto info = dfmbase::InfoFactory::create<AbstractFileInfo>(dirPath, true, &errString);
    if (!info) {
        qInfo() << "create LocalFileInfo error: " << errString;
        return;
    }
    if (info) {
        dirName = info->fileName();

        //解决根目录没有名称问题
        if (dirName.isEmpty() && dir.toLocalFile() == "/") {
            dirName = "/";
        }
    }
}

void CustomActionBuilder::setFocusFile(const QUrl &file)
{
    filePath = file;
    QString errString;
    auto info = dfmbase::InfoFactory::create<AbstractFileInfo>(dirPath, true, &errString);
    if (info) {
        fileFullName = info->fileName();
        //baseName
        if (info->isDir()) {
            fileBaseName = fileFullName;
            return;
        }
        //这里只针对一些常见的后缀处理，暂不针对一些非标准的特殊情况做处理,待后续产品有特殊要求再处理特殊情况
        //suffixForFileName对复式后缀会返回xx.*,比如test.7z.001返回的是7z.*
        //不过在一些非标准的复式后缀判定中，仍可能判定不准确：比如：test.part1.rar被识别成rar
        //隐藏文件：".tar"、".tar.gz"后缀识别成""和".gz"
        //可能无法识别到后缀：如test.run或者.tar
        QString suffix = MimeDatabase::suffixForFileName(fileFullName);
        if (suffix.isEmpty()) {
            fileBaseName = fileFullName;
            return;
        }
        //二次过滤后缀，方式识别到分卷带*的情况
        suffix = this->getCompleteSuffix(fileFullName, suffix);
        fileBaseName = fileFullName.left(fileFullName.length() - suffix.length() - 1);

        //解决 .xx 一类的隐藏文件
        if (fileBaseName.isEmpty())
            fileBaseName = fileFullName;
    }
}

QString CustomActionBuilder::getCompleteSuffix(const QString &fileName, const QString &suf)
{
    QString tempStr;
    if (!suf.contains(".") || suf.isEmpty())
        return suf;
    auto sufLst = suf.split(".");
    if (0 < sufLst.size()) {
        tempStr = sufLst.first();
        int index = fileName.lastIndexOf(tempStr);
        if (index > 0) {
            return fileName.mid(index);
            ;
        }
    }
    return suf;
}

CustomActionDefines::ComboType CustomActionBuilder::checkFileCombo(const QList<QUrl> &files)
{
    int fileCount = 0;
    int dirCount = 0;
    for (const QUrl &file : files) {
        if (file.isEmpty())
            continue;

        QString errString;
        auto info = dfmbase::InfoFactory::create<AbstractFileInfo>(file, true, &errString);
        if (!info)
            continue;

        //目前只判断是否为文件夹
        info->isDir() ? ++dirCount : ++fileCount;

        //文件夹和文件同时存在
        if (dirCount > 0 && fileCount > 0)
            return CustomActionDefines::kFileAndDir;
    }

    //文件
    if (fileCount > 0)
        return fileCount > 1 ? CustomActionDefines::kMultiFiles : CustomActionDefines::kSingleFile;

    //文件夹
    if (dirCount > 0)
        return dirCount > 1 ? CustomActionDefines::kMultiDirs : CustomActionDefines::kSingleDir;

    return CustomActionDefines::kBlankSpace;
}

QList<CustomActionEntry> CustomActionBuilder::matchFileCombo(const QList<CustomActionEntry> &rootActions,
                                                             CustomActionDefines::ComboTypes type)
{
    QList<CustomActionEntry> ret;
    //无自定义菜单项
    if (0 == rootActions.size())
        return ret;

    for (auto it = rootActions.begin(); it != rootActions.end(); ++it) {
        if (it->fileCombo() & type)
            ret << *it;
    }
    return ret;
}

QList<CustomActionEntry> CustomActionBuilder::matchActions(const QList<QUrl> &selects,
                                                           QList<CustomActionEntry> oriActions)
{
    //todo：细化功能颗粒度
    /*
     *根据选中内容、配置项、选中项类型匹配合适的菜单项
     *是否action支持的协议
     *是否action支持的后缀
     *action不支持类型过滤（不加上父类型过滤，todo: 为何不支持项不考虑?）
     *action支持类型过滤(类型过滤要加上父类型一起过滤)
    */

    //具体配置过滤
    for (auto &singleUrl : selects) {
        //协议、后缀
        QString errString;
        AbstractFileInfoPointer fileInfo = dfmbase::InfoFactory::create<AbstractFileInfo>(singleUrl, true, &errString);
        if (!fileInfo) {
            qWarning() << "create selected FileInfo failed: " << singleUrl.toString();
            continue;
        }

        /*
         * 选中文件类型过滤：
         * fileMimeTypes:包括所有父类型的全量类型集合
         * fileMimeTypesNoParent:不包含父类mimetype的集合
         * 目的是在一些应用对文件的识别支持上有差异：比如xlsx的 parentMimeTypes 是application/zip
         * 归档管理器打开则会被作为解压
        */

        QStringList fileMimeTypes;
        QStringList fileMimeTypesNoParent;
        //        fileMimeTypes.append(fileInfo->mimeType().name());
        //        fileMimeTypes.append(fileInfo->mimeType().aliases());
        //        const QMimeType &mt = fileInfo->mimeType();
        //        fileMimeTypesNoParent = fileMimeTypes;
        //        appendParentMimeType(mt.parentMimeTypes(), fileMimeTypes);
        //        fileMimeTypes.removeAll({});
        //        fileMimeTypesNoParent.removeAll({});

        appendAllMimeTypes(fileInfo, fileMimeTypesNoParent, fileMimeTypes);
        for (auto it = oriActions.begin(); it != oriActions.end();) {
            CustomActionEntry &tempAction = *it;
            //协议，后缀
            if (!isSchemeSupport(tempAction, singleUrl) || !isSuffixSupport(tempAction, singleUrl)) {
                it = oriActions.erase(it);   //不支持的action移除
                continue;
            }

            //不支持的mimetypes,使用不包含父类型的mimetype集合过滤
            if (isMimeTypeMatch(fileMimeTypesNoParent, tempAction.excludeMimeTypes())) {
                it = oriActions.erase(it);
                continue;
            }

            // MimeType在原有oem中，未指明或Mimetype=*都作为支持所有类型
            if (tempAction.mimeTypes().isEmpty()) {
                ++it;
                continue;
            }

            //支持的mimetype,使用包含父类型的mimetype集合过滤
            QStringList supportMimeTypes = tempAction.mimeTypes();
            supportMimeTypes.removeAll({});
            auto match = isMimeTypeMatch(fileMimeTypes, supportMimeTypes);

// 在自定义右键菜中有作用域限制，此类情况不显示自定义菜单，故可屏蔽，若后续有作用域的调整再考虑是否开放
#if 0
            //部分mtp挂载设备目录下文件属性不符合规范(普通目录mimetype被认为是octet-stream)，暂时做特殊处理
            if (singleUrl.path().contains("/mtp:host") && supportMimeTypes.contains("application/octet-stream") && fileMimeTypes.contains("application/octet-stream"))
                match = false;
#endif
            if (!match) {
                it = oriActions.erase(it);
                continue;
            }
            ++it;
        }
    }

    return oriActions;
}

QPair<QString, QStringList> CustomActionBuilder::makeCommand(const QString &cmd, CustomActionDefines::ActionArg arg, const QUrl &dir, const QUrl &foucs, const QList<QUrl> &files)
{
    QPair<QString, QStringList> ret;
    auto args = splitCommand(cmd);
    if (args.isEmpty()) {
        return ret;
    }

    //执行程序
    ret.first = args.takeFirst();
    //无参数
    if (args.isEmpty()) {
        return ret;
    }

    auto replace = [=](QStringList &args, const QString &before, const QString &after) {
        QStringList rets;
        while (!args.isEmpty()) {
            QString arg = args.takeFirst();
            //找到在参数中第一个有效的before匹配值，并替换为after。之后的不在处理
            int index = arg.indexOf(before);
            if (index >= 0) {
                rets << arg.replace(index, before.size(), after);
                rets << args;
                args.clear();
            } else {
                rets << arg;
            }
        }
        return rets;
    };

    auto replaceList = [=](QStringList &args, const QString &before, const QStringList &after) {
        QStringList rets;
        while (!args.isEmpty()) {
            QString arg = args.takeFirst();
            //仅支持独立参数，有其它组合的不处理
            if (arg == before) {
                //放入文件路径
                rets << after;
                //放入原参数
                rets << args;
                args.clear();
            } else {
                rets << arg;
            }
        }
        return rets;
    };

    //url转为文件路径
    auto urlListToLocalFile = [](const QList<QUrl> &files) {
        QStringList rets;
        for (auto it = files.begin(); it != files.end(); ++it) {
            rets << it->toLocalFile();
        }
        return rets;
    };

    //url字符串
    auto urlListToString = [](const QList<QUrl> &files) {
        QStringList rets;
        for (auto it = files.begin(); it != files.end(); ++it) {
            rets << it->toString();
        }
        return rets;
    };

    //传参
    switch (arg) {
    case CustomActionDefines::kDirPath:
        ret.second = replace(args, CustomActionDefines::kStrActionArg[arg], dir.toLocalFile());
        break;
    case CustomActionDefines::kFilePath:
        ret.second = replace(args, CustomActionDefines::kStrActionArg[arg], foucs.toLocalFile());
        break;
    case CustomActionDefines::kFilePaths:
        ret.second = replaceList(args, CustomActionDefines::kStrActionArg[arg], urlListToLocalFile(files));
        break;
    case CustomActionDefines::kUrlPath:
        ret.second = replace(args, CustomActionDefines::kStrActionArg[arg], foucs.toString());
        break;
    case CustomActionDefines::kUrlPaths:
        ret.second = replaceList(args, CustomActionDefines::kStrActionArg[arg], urlListToString(files));
        break;
    default:
        ret.second = args;
        break;
    }
    return ret;
}

QStringList CustomActionBuilder::splitCommand(const QString &cmd)
{
    QStringList args;
    bool inQuote = false;

    QString arg;
    for (int i = 0; i < cmd.count(); i++) {
        const bool isEnd = (cmd.size() == (i + 1));

        const QChar &ch = cmd.at(i);
        //引号
        const bool isQuote = (ch == QLatin1Char('\'') || ch == QLatin1Char('\"'));

        //遇到引号或者最后一个字符
        if (!isEnd && isQuote) {
            //进入引号内或退出引号
            inQuote = !inQuote;
        } else {
            //处于引号中或者非空格作为一个参数
            if ((!ch.isSpace() || inQuote) && !isQuote) {
                arg.append(ch);
            }

            //遇到空格且不再引号中解出一个单独参数
            if ((ch.isSpace() && !inQuote) || isEnd) {
                if (!arg.isEmpty()) {
                    args << arg;
                }
                arg.clear();
            }
        }
    }
    return args;
}

bool CustomActionBuilder::isMimeTypeSupport(const QString &mt, const QStringList &fileMimeTypes)
{
    foreach (const QString &fmt, fileMimeTypes) {
        if (fmt.contains(mt, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool CustomActionBuilder::isMimeTypeMatch(const QStringList &fileMimeTypes, const QStringList &supportMimeTypes)
{
    bool match = false;
    for (const QString &mt : supportMimeTypes) {
        if (fileMimeTypes.contains(mt, Qt::CaseInsensitive)) {
            match = true;
            break;
        }

        int starPos = mt.indexOf("*");
        if (starPos >= 0 && isMimeTypeSupport(mt.left(starPos), fileMimeTypes)) {
            match = true;
            break;
        }
    }
    return match;
}

bool CustomActionBuilder::isSchemeSupport(const CustomActionEntry &action, const QUrl &url)
{
    // X-DFM-SupportSchemes not exist
    auto supportList = action.surpportSchemes();
    if (supportList.contains("*") || supportList.isEmpty())
        return true;   //支持所有协议: 未特殊指明X-DFM-SupportSchemes或者"X-DFM-SupportSchemes=*"
    return supportList.contains(url.scheme(), Qt::CaseInsensitive);
}

bool CustomActionBuilder::isSuffixSupport(const CustomActionEntry &action, const QUrl &url)
{
    QString errString;
    AbstractFileInfoPointer fileInfo = dfmbase::InfoFactory::create<AbstractFileInfo>(url, true, &errString);
    auto supportList = action.supportStuffix();
    if (!fileInfo || fileInfo->isDir() || supportList.isEmpty() || supportList.contains("*")) {
        return true;   //未特殊指明支持项或者包含*为支持所有
    }
    QFileInfo info(url.toLocalFile());

    //例如： 7z.001,7z.002, 7z.003 ... 7z.xxx
    QString cs = info.completeSuffix();
    if (supportList.contains(cs, Qt::CaseInsensitive)) {
        return true;
    }

    bool match = false;
    for (const QString &suffix : supportList) {
        auto tempSuffix = suffix;
        int endPos = tempSuffix.lastIndexOf("*");   // 例如：7z.*
        if (endPos >= 0 && cs.length() > endPos && tempSuffix.left(endPos) == cs.left(endPos)) {
            match = true;
            break;
        }
    }
    return match;
}

void CustomActionBuilder::appendAllMimeTypes(const AbstractFileInfoPointer &fileInfo, QStringList &noParentmimeTypes, QStringList &allMimeTypes)
{
    noParentmimeTypes.append(fileInfo->fileMimeType().name());
    noParentmimeTypes.append(fileInfo->fileMimeType().aliases());
    const QMimeType &mt = fileInfo->fileMimeType();
    allMimeTypes = noParentmimeTypes;
    appendParentMimeType(mt.parentMimeTypes(), allMimeTypes);
    noParentmimeTypes.removeAll({});
    allMimeTypes.removeAll({});
}

void CustomActionBuilder::appendParentMimeType(const QStringList &parentmimeTypes, QStringList &mimeTypes)
{
    if (parentmimeTypes.size() == 0)
        return;

    for (const QString &mtName : parentmimeTypes) {
        QMimeDatabase db;
        QMimeType mt = db.mimeTypeForName(mtName);
        mimeTypes.append(mt.name());
        mimeTypes.append(mt.aliases());
        QStringList pmts = mt.parentMimeTypes();
        appendParentMimeType(pmts, mimeTypes);
    }
}

QAction *CustomActionBuilder::createMenu(const CustomActionData &actionData, QWidget *parentForSubmenu) const
{
    //createAction 构造action 图标等, 把关于构造action参数放在createAction中
    QAction *action = createAciton(actionData);
    QMenu *menu = new QMenu(parentForSubmenu);
    menu->setToolTipsVisible(true);

    action->setMenu(menu);
    action->setProperty(CustomActionDefines::kCustomActionFlag, true);

    //子项,子项的顺序由解析器保证
    QList<CustomActionData> subActions = actionData.acitons();
    for (auto it = subActions.begin(); it != subActions.end(); ++it) {
        QAction *ba = buildAciton(*it, parentForSubmenu);
        if (!ba)
            continue;

        auto separator = it->separator();
        //上分割线
        if (separator & CustomActionDefines::kTop) {
            const QList<QAction *> &actionList = menu->actions();
            if (!actionList.isEmpty()) {
                auto lastAction = menu->actions().last();

                //不是分割线则插入
                if (!lastAction->isSeparator()) {
                    menu->addSeparator();
                }
            }
        }

        ba->setParent(menu);
        menu->addAction(ba);

        //下分割线
        if ((separator & CustomActionDefines::kBottom) && ((it + 1) != subActions.end())) {
            menu->addSeparator();
        }
    }

    return action;
}

QAction *CustomActionBuilder::createAciton(const CustomActionData &actionData) const
{
    QAction *action = new QAction;
    action->setProperty(CustomActionDefines::kCustomActionFlag, true);

    //执行动作
    action->setProperty(CustomActionDefines::kCustomActionCommand, actionData.command());
    action->setProperty(CustomActionDefines::kCustomActionCommandArgFlag, actionData.commandArg());

    //标题
    {
        const QString &&name = makeName(actionData.name(), actionData.nameArg());
        //TODO width是临时值，最终效果需设计定义
        const QString &&elidedName = fm.elidedText(name, Qt::ElideMiddle, 150);
        action->setText(elidedName);
        if (elidedName != name)
            action->setToolTip(name);
    }
// 产品变更暂不考虑图标
#if 0
    //图标
    const QString &iconName = actionData.icon();
    if (!iconName.isEmpty()) {
        const QIcon &&icon = getIcon(iconName);
        if (!icon.isNull())
            action->setIcon(icon);
    }
#endif
    return action;
}

QIcon CustomActionBuilder::getIcon(const QString &iconName) const
{
    QIcon ret;

    //通过路径获取图标
    QFileInfo info(iconName.startsWith("~") ? (QDir::homePath() + iconName.mid(1)) : iconName);

    if (!info.exists())
        info.setFile(QUrl::fromUserInput(iconName).toLocalFile());

    if (info.exists()) {
        ret = QIcon(info.absoluteFilePath());
    }

    //从主题获取
    if (ret.isNull()) {
        ret = QIcon::fromTheme(iconName);
    }

    return ret;
}

QString CustomActionBuilder::makeName(const QString &name, CustomActionDefines::ActionArg arg) const
{
    auto replace = [](QString input, const QString &before, const QString &after) {
        QString ret = input;
        int index = input.indexOf(before);
        if (index >= 0) {
            ret = input.replace(index, before.size(), after);
        }
        return ret;
    };

    QString ret;
    switch (arg) {
    case CustomActionDefines::kDirName:
        ret = replace(name, CustomActionDefines::kStrActionArg[arg], dirName);
        break;
    case CustomActionDefines::kBaseName:
        ret = replace(name, CustomActionDefines::kStrActionArg[arg], fileBaseName);
        break;
    case CustomActionDefines::kFileName:
        ret = replace(name, CustomActionDefines::kStrActionArg[arg], fileFullName);
        break;
    default:
        ret = name;
        break;
    }
    return ret;
}

DSC_END_NAMESPACE
